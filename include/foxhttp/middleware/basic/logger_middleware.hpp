#pragma once

#include <spdlog/spdlog.h>

#include <chrono>
#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/server/request_context.hpp>
#include <memory>
#include <string>

namespace foxhttp::middleware {

enum class LogLevel {
  Trace = 0,
  Debug = 1,
  Info = 2,
  Warn = 3,
  Error = 4,
  Critical = 5
};

enum class LogFormat {
  Simple,
  Detailed,
  Json,
  Apache
};

class LoggerMiddleware : public PriorityMiddleware<MiddlewarePriority::High> {
 public:
  explicit LoggerMiddleware(const std::string &name = "LoggerMiddleware", LogLevel level = LogLevel::Info,
                             LogFormat format = LogFormat::Detailed, const std::string &log_file = {},
                             bool enable_console = true);

  std::string name() const override;

  void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
  void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override;

  void set_log_level(LogLevel level);
  void set_log_format(LogFormat format);
  void set_log_file(const std::string &file);
  void set_console_enabled(bool enabled);

 private:
  void setup_logger();
  std::string generate_request_id();
  void log_request_start(const RequestContext &ctx, const std::string &request_id);
  void log_request_complete(const RequestContext &ctx, const http::response<http::string_body> &res,
                            const std::string &request_id, std::chrono::microseconds duration, bool async);
  void log_request_error(const RequestContext &ctx, const http::response<http::string_body> &res,
                         const std::string &request_id, std::chrono::microseconds duration, const std::string &error);
  void log_request_timeout(const RequestContext &ctx, const http::response<http::string_body> &res,
                           const std::string &request_id, std::chrono::microseconds duration);
  void log_request_stopped(const RequestContext &ctx, const http::response<http::string_body> &res,
                           const std::string &request_id, std::chrono::microseconds duration);
  std::string get_apache_timestamp();

 private:
  std::string name_;
  LogLevel level_;
  LogFormat format_;
  std::string log_file_;
  bool enable_console_;
  std::shared_ptr<spdlog::logger> logger_;
};

}  // namespace foxhttp::middleware
