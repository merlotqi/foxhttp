#include <foxhttp/middleware/basic/functional_middleware.hpp>
#include <foxhttp/middleware/middleware.hpp>

namespace foxhttp::middleware {

/* ----------------------------- MiddlewareStats ---------------------------- */

MiddlewareStats::MiddlewareStats(const MiddlewareStats &other)
    : execution_count(other.execution_count.load()),
      total_execution_time(other.total_execution_time.load()),
      error_count(other.error_count.load()),
      timeout_count(other.timeout_count.load()) {}

MiddlewareStats &MiddlewareStats::operator=(const MiddlewareStats &other) {
  if (this != &other) {
    execution_count.store(other.execution_count.load());
    total_execution_time.store(other.total_execution_time.load());
    error_count.store(other.error_count.load());
    timeout_count.store(other.timeout_count.load());
  }
  return *this;
}

void MiddlewareStats::reset() {
  execution_count = 0;
  total_execution_time = std::chrono::microseconds{0};
  error_count = 0;
  timeout_count = 0;
}

/* ------------------------------- middleware ------------------------------- */

void Middleware::operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next,
                            async_middleware_callback callback) {
  // Default implementation: execute synchronously and call callback
  try {
    (*this)(ctx, res, next);
    callback(MiddlewareResult::Continue, "");
  } catch (const std::exception &e) {
    callback(MiddlewareResult::Error, e.what());
  }
}

// Get middleware priority (default: normal)
MiddlewarePriority Middleware::priority() const { return MiddlewarePriority::Normal; }

// Get middleware name for logging/debugging
std::string Middleware::name() const { return "AnonymousMiddleware"; }

// Check if middleware should execute for this request
bool Middleware::should_execute(RequestContext &ctx) const { return true; }

// Get execution timeout for this middleware (0 = no timeout)
std::chrono::milliseconds Middleware::timeout() const { return std::chrono::milliseconds{0}; }

// Get statistics
MiddlewareStats &Middleware::stats() { return stats_; }
const MiddlewareStats &Middleware::stats() const { return stats_; }

/* -------------------------- ConditionalMiddleware ------------------------- */

ConditionalMiddleware::ConditionalMiddleware(std::shared_ptr<Middleware> middleware, condition_func condition)
    : mw_(std::move(middleware)), condition_(std::move(condition)) {}

void ConditionalMiddleware::operator()(RequestContext &ctx, http::response<http::string_body> &res,
                                        std::function<void()> next) {
  if (condition_(ctx)) {
    (*mw_)(ctx, res, next);
  } else {
    next();
  }
}

void ConditionalMiddleware::operator()(RequestContext &ctx, http::response<http::string_body> &res,
                                        std::function<void()> next, async_middleware_callback callback) {
  if (condition_(ctx)) {
    (*mw_)(ctx, res, next, callback);
  } else {
    next();
    callback(MiddlewareResult::Continue, "");
  }
}

MiddlewarePriority ConditionalMiddleware::priority() const { return mw_->priority(); }
std::string ConditionalMiddleware::name() const { return "Conditional(" + mw_->name() + ")"; }
bool ConditionalMiddleware::should_execute(RequestContext &ctx) const { return condition_(ctx); }
std::chrono::milliseconds ConditionalMiddleware::timeout() const { return mw_->timeout(); }

MiddlewareStats &ConditionalMiddleware::stats() { return mw_->stats(); }
const MiddlewareStats &ConditionalMiddleware::stats() const { return mw_->stats(); }

/* ----------------------------- MiddlewareBuilder ------------------------- */

MiddlewareBuilder::MiddlewareBuilder() {}

MiddlewareBuilder &MiddlewareBuilder::set_name(const std::string &name) {
  name_ = name;
  return *this;
}

MiddlewareBuilder &MiddlewareBuilder::set_priority(MiddlewarePriority priority) {
  priority_ = priority;
  return *this;
}

MiddlewareBuilder &MiddlewareBuilder::set_timeout(std::chrono::milliseconds timeout) {
  timeout_ = timeout;
  return *this;
}

MiddlewareBuilder &MiddlewareBuilder::set_sync_func(sync_func func) {
  sync_func_ = std::move(func);
  return *this;
}

MiddlewareBuilder &MiddlewareBuilder::set_async_func(async_func func) {
  async_func_ = std::move(func);
  return *this;
}

MiddlewareBuilder &MiddlewareBuilder::set_condition(condition_func condition) {
  condition_ = std::move(condition);
  return *this;
}

std::shared_ptr<Middleware> MiddlewareBuilder::build() {
  return std::make_shared<FunctionalMiddleware>(name_, sync_func_, async_func_, condition_, priority_, timeout_);
}

}  // namespace foxhttp::middleware
