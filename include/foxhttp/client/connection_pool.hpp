#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>

namespace foxhttp::client {

/// Represents a reusable TCP connection
class connection {
 public:
  using socket_type = boost::asio::ip::tcp::socket;

  explicit connection(socket_type socket);
  ~connection() = default;

  /// Get the underlying socket
  socket_type &socket() { return socket_; }

  /// Check if connection is still alive (socket is open)
  bool is_alive() const;

  /// Mark connection as in use
  void mark_in_use();

  /// Mark connection as idle
  void mark_idle();

  /// Check if connection is currently in use
  bool is_in_use() const { return in_use_; }

  /// Get last activity time
  std::chrono::steady_clock::time_point last_activity() const { return last_activity_; }

 private:
  socket_type socket_;
  bool in_use_{false};
  std::chrono::steady_clock::time_point last_activity_;
};

/// Connection pool for reusing TCP connections
class connection_pool : public std::enable_shared_from_this<connection_pool> {
 public:
  /// Constructor
  /// @param max_size Maximum number of connections to pool
  /// @param idle_timeout How long to keep idle connections alive
  /// @param cleanup_interval How often to cleanup expired connections
  explicit connection_pool(std::size_t max_size = 100, std::chrono::seconds idle_timeout = std::chrono::seconds(60),
                           std::chrono::seconds cleanup_interval = std::chrono::seconds(10));

  ~connection_pool();

  /// Start periodic cleanup timer
  /// @param io IO context for the timer
  void start_cleanup_timer(boost::asio::io_context &io);

  /// Acquire a connection from the pool
  /// @param host Target hostname
  /// @param port Target port
  /// @return Connection if available, nullptr otherwise
  std::shared_ptr<connection> acquire(const std::string &host, uint16_t port);

  /// Release a connection back to the pool
  /// @param conn Connection to release
  /// @param host Target hostname
  /// @param port Target port
  void release(std::shared_ptr<connection> conn, const std::string &host, uint16_t port);

  /// Get total number of connections in pool
  std::size_t size() const;

  /// Get number of active (in-use) connections
  std::size_t active_count() const;

  /// Clear all connections from the pool
  void clear();

 private:
  /// Generate a key for host:port combination
  static std::string make_key(const std::string &host, uint16_t port);

  /// Cleanup expired idle connections
  void cleanup_expired();

  /// Remove expired connections from a specific pool
  void cleanup_pool(std::queue<std::shared_ptr<connection>> &pool);

 private:
  std::size_t max_size_;
  std::chrono::seconds idle_timeout_;
  std::chrono::seconds cleanup_interval_;

  mutable std::mutex mutex_;
  std::unordered_map<std::string, std::queue<std::shared_ptr<connection>>> pools_;
  std::size_t total_connections_{0};
  std::size_t active_connections_{0};

  /// Periodic cleanup timer
  std::unique_ptr<boost::asio::steady_timer> cleanup_timer_;

  /// IO context pointer for timer reinitialization
  boost::asio::io_context *io_context_{nullptr};
};

}  // namespace foxhttp::client
