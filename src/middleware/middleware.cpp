/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <foxhttp/middleware/basic/functional_middleware.hpp>
#include <foxhttp/middleware/middleware.hpp>

namespace foxhttp {

/* ----------------------------- middleware_stats ---------------------------- */

middleware_stats::middleware_stats(const middleware_stats &other)
    : execution_count(other.execution_count.load()),
      total_execution_time(other.total_execution_time.load()),
      error_count(other.error_count.load()),
      timeout_count(other.timeout_count.load()) {}

middleware_stats &middleware_stats::operator=(const middleware_stats &other) {
  if (this != &other) {
    execution_count.store(other.execution_count.load());
    total_execution_time.store(other.total_execution_time.load());
    error_count.store(other.error_count.load());
    timeout_count.store(other.timeout_count.load());
  }
  return *this;
}

void middleware_stats::reset() {
  execution_count = 0;
  total_execution_time = std::chrono::microseconds{0};
  error_count = 0;
  timeout_count = 0;
}

/* ------------------------------- middleware ------------------------------- */

void middleware::operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next,
                            async_middleware_callback callback) {
  // Default implementation: execute synchronously and call callback
  try {
    (*this)(ctx, res, next);
    callback(middleware_result::continue_, "");
  } catch (const std::exception &e) {
    callback(middleware_result::error, e.what());
  }
}

// Get middleware priority (default: normal)
middleware_priority middleware::priority() const { return middleware_priority::normal; }

// Get middleware name for logging/debugging
std::string middleware::name() const { return "AnonymousMiddleware"; }

// Check if middleware should execute for this request
bool middleware::should_execute(request_context &ctx) const { return true; }

// Get execution timeout for this middleware (0 = no timeout)
std::chrono::milliseconds middleware::timeout() const { return std::chrono::milliseconds{0}; }

// Get statistics
middleware_stats &middleware::stats() { return stats_; }
const middleware_stats &middleware::stats() const { return stats_; }

/* -------------------------- conditional_middleware ------------------------- */

conditional_middleware::conditional_middleware(std::shared_ptr<middleware> middleware, condition_func condition)
    : mw_(std::move(middleware)), condition_(std::move(condition)) {}

void conditional_middleware::operator()(request_context &ctx, http::response<http::string_body> &res,
                                        std::function<void()> next) {
  if (condition_(ctx)) {
    (*mw_)(ctx, res, next);
  } else {
    next();
  }
}

void conditional_middleware::operator()(request_context &ctx, http::response<http::string_body> &res,
                                        std::function<void()> next, async_middleware_callback callback) {
  if (condition_(ctx)) {
    (*mw_)(ctx, res, next, callback);
  } else {
    next();
    callback(middleware_result::continue_, "");
  }
}

middleware_priority conditional_middleware::priority() const { return mw_->priority(); }
std::string conditional_middleware::name() const { return "Conditional(" + mw_->name() + ")"; }
bool conditional_middleware::should_execute(request_context &ctx) const { return condition_(ctx); }
std::chrono::milliseconds conditional_middleware::timeout() const { return mw_->timeout(); }

middleware_stats &conditional_middleware::stats() { return mw_->stats(); }
const middleware_stats &conditional_middleware::stats() const { return mw_->stats(); }

/* --------------------------- midddleware_builder -------------------------- */

midddleware_builder::midddleware_builder() {}

midddleware_builder &midddleware_builder::set_name(const std::string &name) {
  name_ = name;
  return *this;
}

midddleware_builder &midddleware_builder::set_priority(middleware_priority priority) {
  priority_ = priority;
  return *this;
}

midddleware_builder &midddleware_builder::set_timeout(std::chrono::milliseconds timeout) {
  timeout_ = timeout;
  return *this;
}

midddleware_builder &midddleware_builder::set_sync_func(sync_func func) {
  sync_func_ = std::move(func);
  return *this;
}

midddleware_builder &midddleware_builder::set_async_func(async_func func) {
  async_func_ = std::move(func);
  return *this;
}

midddleware_builder &midddleware_builder::set_condition(condition_func condition) {
  condition_ = std::move(condition);
  return *this;
}

std::shared_ptr<middleware> midddleware_builder::build() {
  return std::make_shared<functional_middleware>(name_, sync_func_, async_func_, condition_, priority_, timeout_);
}

}  // namespace foxhttp
