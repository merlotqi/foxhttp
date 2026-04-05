#pragma once

#include <boost/algorithm/string.hpp>
#include <foxhttp/middleware/basic/cors_middleware.hpp>
#include <foxhttp/middleware/basic/logger_middleware.hpp>
#include <foxhttp/middleware/basic/response_time_middleware.hpp>
#include <foxhttp/middleware/middleware.hpp>
#include <functional>
#include <string>

namespace foxhttp::middleware {

class FunctionalMiddleware : public Middleware {
 public:
  using sync_func = std::function<void(RequestContext &, http::response<http::string_body> &, std::function<void()>)>;
  using async_func = std::function<void(RequestContext &, http::response<http::string_body> &, std::function<void()>,
                                        async_middleware_callback)>;
  using condition_func = std::function<bool(RequestContext &)>;

  FunctionalMiddleware(const std::string &name, sync_func sync_func, async_func async_func = nullptr,
                        condition_func condition = nullptr, MiddlewarePriority priority = MiddlewarePriority::Normal,
                        std::chrono::milliseconds timeout = std::chrono::milliseconds{0});

  std::string name() const override;
  MiddlewarePriority priority() const override;
  bool should_execute(RequestContext &ctx) const override;
  std::chrono::milliseconds timeout() const override;

  void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
  void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override;

 private:
  std::string name_;
  sync_func sync_func_;
  async_func async_func_;
  condition_func condition_;
  MiddlewarePriority priority_;
  std::chrono::milliseconds timeout_;
};

}  // namespace foxhttp::middleware
