#pragma once

#include <foxhttp/middleware/middleware.hpp>
#include <string>

namespace foxhttp::middleware {

class CorsMiddleware : public PriorityMiddleware<MiddlewarePriority::High> {
 public:
  explicit CorsMiddleware(const std::string &origin = "*",
                           const std::string &methods = "GET, POST, PUT, DELETE, OPTIONS",
                           const std::string &headers = "Content-Type, Authorization", bool allow_credentials = false,
                           long max_age = 86400);
  std::string name() const override;

  void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
  void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override;

 private:
  std::string origin_;
  std::string methods_;
  std::string headers_;
  bool allow_credentials_;
  long max_age_;
};

}  // namespace foxhttp::middleware
