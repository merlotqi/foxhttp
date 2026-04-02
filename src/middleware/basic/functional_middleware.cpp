#include <foxhttp/middleware/basic/functional_middleware.hpp>

namespace foxhttp::middleware {

/* -------------------------- FunctionalMiddleware -------------------------- */

FunctionalMiddleware::FunctionalMiddleware(const std::string &name, sync_func sync_func, async_func async_func,
                                             condition_func condition, MiddlewarePriority priority,
                                             std::chrono::milliseconds timeout)
    : name_(name),
      sync_func_(std::move(sync_func)),
      async_func_(std::move(async_func)),
      condition_(std::move(condition)),
      priority_(priority),
      timeout_(timeout) {}

std::string FunctionalMiddleware::name() const { return name_; }
MiddlewarePriority FunctionalMiddleware::priority() const { return priority_; }
bool FunctionalMiddleware::should_execute(RequestContext &ctx) const { return condition_ ? condition_(ctx) : true; }
std::chrono::milliseconds FunctionalMiddleware::timeout() const { return timeout_; }

void FunctionalMiddleware::operator()(RequestContext &ctx, http::response<http::string_body> &res,
                                       std::function<void()> next) {
  sync_func_(ctx, res, next);
}

void FunctionalMiddleware::operator()(RequestContext &ctx, http::response<http::string_body> &res,
                                       std::function<void()> next, async_middleware_callback callback) {
  if (async_func_) {
    async_func_(ctx, res, next, callback);
  } else {
    // Fallback to sync implementation
    Middleware::operator()(ctx, res, next, callback);
  }
}

}  // namespace foxhttp::middleware
