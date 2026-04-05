#include <foxhttp/server/io_context_pool.hpp>

namespace foxhttp::server {
IoContextPool::IoContextPool(std::size_t pool_size) : next_io_context_(0), pool_size_(pool_size), stopped_(false) {
  if (pool_size_ == 0) throw std::runtime_error("IoContextPool size cannot be zero");

  for (std::size_t i = 0; i < pool_size_; ++i) {
    auto io_ctx = std::make_shared<boost::asio::io_context>();
    io_contexts_.push_back(io_ctx);
    works_.push_back(std::make_shared<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
        io_ctx->get_executor()));
  }
}

void IoContextPool::run_blocking() {
  for (std::size_t i = 0; i < pool_size_; ++i) {
    threads_.emplace_back([io = io_contexts_[i]] { io->run(); });
  }

  for (auto &t : threads_) {
    if (t.joinable()) {
      t.join();
    }
  }
  threads_.clear();
}

void IoContextPool::start() { run_blocking(); }

boost::asio::io_context &IoContextPool::get_io_context() {
  auto index = next_io_context_.fetch_add(1, std::memory_order_relaxed) % pool_size_;
  return *io_contexts_[index];
}

void IoContextPool::stop() {
  if (stopped_.exchange(true)) return;

  for (auto &work : works_) work->reset();

  for (auto &io : io_contexts_) io->stop();
}

std::size_t IoContextPool::size() const { return pool_size_; }

IoContextPool::~IoContextPool() {
  stop();

  for (auto &t : threads_) {
    if (t.joinable()) t.join();
  }
}

}  // namespace foxhttp::server
