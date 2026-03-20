/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <spdlog/spdlog.h>

#include <chrono>
#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/server/request_context.hpp>
#include <memory>
#include <string>

namespace foxhttp {

enum class log_level {
  trace = 0,
  debug = 1,
  info = 2,
  warn = 3,
  error = 4,
  critical = 5
};

enum class log_format {
  simple,
  detailed,
  json,
  apache
};

class logger_middleware : public priority_middleware<middleware_priority::high> {
 public:
  explicit logger_middleware(const std::string &name = "LoggerMiddleware", log_level level = log_level::info,
                             log_format format = log_format::detailed, const std::string &log_file = {},
                             bool enable_console = true);

  std::string name() const override;

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override;

  void set_log_level(log_level level);
  void set_log_format(log_format format);
  void set_log_file(const std::string &file);
  void set_console_enabled(bool enabled);

 private:
  void setup_logger();
  std::string generate_request_id();
  void log_request_start(const request_context &ctx, const std::string &request_id);
  void log_request_complete(const request_context &ctx, const http::response<http::string_body> &res,
                             const std::string &request_id, std::chrono::microseconds duration, bool async);
  void log_request_error(const request_context &ctx, const http::response<http::string_body> &res,
                          const std::string &request_id, std::chrono::microseconds duration, const std::string &error);
  void log_request_timeout(const request_context &ctx, const http::response<http::string_body> &res,
                            const std::string &request_id, std::chrono::microseconds duration);
  void log_request_stopped(const request_context &ctx, const http::response<http::string_body> &res,
                            const std::string &request_id, std::chrono::microseconds duration);
  std::string get_apache_timestamp();

 private:
  std::string name_;
  log_level level_;
  log_format format_;
  std::string log_file_;
  bool enable_console_;
  std::shared_ptr<spdlog::logger> logger_;
};

}  // namespace foxhttp
