/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <foxhttp/server/io_context_pool.hpp>

namespace foxhttp {
io_context_pool::io_context_pool(std::size_t pool_size) : next_io_context_(0), pool_size_(pool_size), stopped_(false) {
  if (pool_size_ == 0) throw std::runtime_error("io_context_pool size cannot be zero");

  for (std::size_t i = 0; i < pool_size_; ++i) {
    auto io_ctx = std::make_shared<boost::asio::io_context>();
    io_contexts_.push_back(io_ctx);
    works_.push_back(std::make_shared<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
        io_ctx->get_executor()));
  }
}

void io_context_pool::start() {
  for (std::size_t i = 0; i < pool_size_; ++i) {
    threads_.emplace_back([io = io_contexts_[i]] { io->run(); });
  }

  for (auto &t : threads_) {
    if (t.joinable()) t.join();
  }
}

boost::asio::io_context &io_context_pool::get_io_context() {
  auto index = next_io_context_.fetch_add(1, std::memory_order_relaxed) % pool_size_;
  return *io_contexts_[index];
}

void io_context_pool::stop() {
  if (stopped_.exchange(true)) return;

  for (auto &work : works_) work->reset();

  for (auto &io : io_contexts_) io->stop();
}

std::size_t io_context_pool::size() const { return pool_size_; }

io_context_pool::~io_context_pool() {
  stop();

  for (auto &t : threads_) {
    if (t.joinable()) t.join();
  }
}

}  // namespace foxhttp
