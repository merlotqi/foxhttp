#include <spdlog/spdlog.h>

#include <boost/asio/post.hpp>
#include <foxhttp/client/connection_pool.hpp>

namespace foxhttp::client {

/* -------------------------------------------------------------------------- */
/*                          connection implementation                         */
/* -------------------------------------------------------------------------- */
connection::connection(socket_type socket)
    : socket_(std::move(socket)), in_use_(false), last_activity_(std::chrono::steady_clock::now()) {}

bool connection::is_alive() const { return socket_.is_open(); }

void connection::mark_in_use() {
  in_use_ = true;
  last_activity_ = std::chrono::steady_clock::now();
}

void connection::mark_idle() {
  in_use_ = false;
  last_activity_ = std::chrono::steady_clock::now();
}

/* -------------------------------------------------------------------------- */
/*                       connection_pool implementation                       */
/* -------------------------------------------------------------------------- */
connection_pool::connection_pool(std::size_t max_size, std::chrono::seconds idle_timeout,
                                 std::chrono::seconds cleanup_interval)
    : max_size_(max_size), idle_timeout_(idle_timeout), cleanup_interval_(cleanup_interval) {}

connection_pool::~connection_pool() {
  if (cleanup_timer_) {
    try {
      cleanup_timer_->cancel();
    } catch (const boost::system::system_error &e) {
      spdlog::warn("Failed to cancel cleanup timer: {}", e.what());
    }
  }
  clear();
}

void connection_pool::start_cleanup_timer(boost::asio::io_context &io) {
  io_context_ = &io;
  cleanup_timer_ = std::make_unique<boost::asio::steady_timer>(io, cleanup_interval_);

  std::weak_ptr<connection_pool> weak_self = shared_from_this();
  cleanup_timer_->async_wait([weak_self](const boost::system::error_code &ec) {
    if (ec == boost::asio::error::operation_aborted) {
      return;
    }

    auto self = weak_self.lock();
    if (self && self->io_context_) {
      self->cleanup_expired();
      self->start_cleanup_timer(*self->io_context_);
    }
  });

  spdlog::debug("connection_pool: started cleanup timer with interval {}s", cleanup_interval_.count());
}

std::shared_ptr<connection> connection_pool::acquire(const std::string &host, uint16_t port) {
  std::lock_guard<std::mutex> lock(mutex_);

  const auto key = make_key(host, port);
  auto &pool = pools_[key];

  // Try to find a usable connection
  while (!pool.empty()) {
    auto conn = pool.front();
    pool.pop();

    // Check if connection is still alive
    if (conn && conn->is_alive()) {
      conn->mark_in_use();
      ++active_connections_;
      spdlog::debug("connection_pool: acquired connection for {}:{}", host, port);
      return conn;
    }

    // Connection is dead, decrement total count
    --total_connections_;
  }

  // No available connection
  spdlog::debug("connection_pool: no available connection for {}:{}", host, port);
  return nullptr;
}

void connection_pool::release(std::shared_ptr<connection> conn, const std::string &host, uint16_t port) {
  if (!conn) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  const auto key = make_key(host, port);
  auto &pool = pools_[key];

  // Mark connection as idle
  conn->mark_idle();
  --active_connections_;

  // Check if connection is still alive
  if (!conn->is_alive()) {
    --total_connections_;
    spdlog::debug("connection_pool: discarded dead connection for {}:{}", host, port);
    return;
  }

  // Check if pool is full
  if (total_connections_ >= max_size_) {
    --total_connections_;
    spdlog::debug("connection_pool: pool full, discarded connection for {}:{}", host, port);
    return;
  }

  // Add connection back to pool
  pool.push(conn);
  spdlog::debug("connection_pool: released connection for {}:{}, pool size: {}", host, port, pool.size());
}

std::size_t connection_pool::size() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return total_connections_;
}

std::size_t connection_pool::active_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return active_connections_;
}

void connection_pool::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  pools_.clear();
  total_connections_ = 0;
  active_connections_ = 0;
  spdlog::debug("connection_pool: cleared all connections");
}

std::string connection_pool::make_key(const std::string &host, uint16_t port) {
  return host + ":" + std::to_string(port);
}

void connection_pool::cleanup_expired() {
  std::lock_guard<std::mutex> lock(mutex_);

  const auto now = std::chrono::steady_clock::now();

  for (auto it = pools_.begin(); it != pools_.end();) {
    auto &pool = it->second;
    cleanup_pool(pool);

    // Remove empty pools
    if (pool.empty()) {
      it = pools_.erase(it);
    } else {
      ++it;
    }
  }

  spdlog::debug("connection_pool: cleanup completed, total connections: {}", total_connections_);
}

void connection_pool::cleanup_pool(std::queue<std::shared_ptr<connection>> &pool) {
  const auto now = std::chrono::steady_clock::now();
  std::queue<std::shared_ptr<connection>> valid_connections;

  while (!pool.empty()) {
    auto conn = pool.front();
    pool.pop();

    if (!conn || !conn->is_alive()) {
      // Connection is dead
      --total_connections_;
      continue;
    }

    // Check if connection has been idle too long
    const auto idle_time = now - conn->last_activity();
    if (idle_time > idle_timeout_) {
      --total_connections_;
      spdlog::debug("connection_pool: removed idle connection");
      continue;
    }

    valid_connections.push(conn);
  }

  pool = std::move(valid_connections);
}

}  // namespace foxhttp::client
