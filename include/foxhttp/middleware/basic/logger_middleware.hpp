/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <chrono>
#include <foxhttp/core/request_context.hpp>
#include <foxhttp/middleware/middleware.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>

namespace foxhttp {

enum class LogLevel
{
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    CRITICAL = 5
};

enum class LogFormat
{
    SIMPLE,  // Simple one-line format
    DETAILED,// Detailed multi-line format
    JSON,    // JSON format for structured logging
    APACHE   // Apache Common Log Format
};

class LoggerMiddleware : public PriorityMiddleware<middleware_priority::high>
{
public:
    explicit LoggerMiddleware(const std::string &name = "LoggerMiddleware", LogLevel level = LogLevel::INFO,
                              LogFormat format = LogFormat::DETAILED, const std::string &log_file = "",
                              bool enable_console = true);

    std::string name() const override;

    void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
    void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next,
                    AsyncMiddlewareCallback callback) override;

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

}// namespace foxhttp
