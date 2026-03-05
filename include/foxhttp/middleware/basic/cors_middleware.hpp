/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <foxhttp/middleware/middleware.hpp>
#include <string>

namespace foxhttp {

class cors_middleware : public priority_middleware<middleware_priority::high> {
 public:
  explicit cors_middleware(const std::string &origin = "*",
                           const std::string &methods = "GET, POST, PUT, DELETE, OPTIONS",
                           const std::string &headers = "Content-Type, Authorization", bool allow_credentials = false,
                           long max_age = 86400);
  std::string name() const override;

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override;

 private:
  std::string origin_;
  std::string methods_;
  std::string headers_;
  bool allow_credentials_;
  long max_age_;
};

}  // namespace foxhttp
