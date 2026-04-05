#pragma once

#include <filesystem>
#include <foxhttp/middleware/basic/body_parser_middleware.hpp>
#include <foxhttp/middleware/basic/cors_middleware.hpp>
#include <foxhttp/middleware/basic/logger_middleware.hpp>
#include <foxhttp/middleware/basic/response_time_middleware.hpp>
#include <foxhttp/middleware/basic/static_middleware.hpp>
#include <memory>
#include <string>

namespace foxhttp::middleware {

/// Factory helpers returning @c std::shared_ptr<Middleware>.
namespace factories {

inline std::shared_ptr<Middleware> create_logger_middleware(const std::string &name = "LoggerMiddleware",
                                                              LogLevel level = LogLevel::Info,
                                                              LogFormat format = LogFormat::Detailed,
                                                              const std::string &log_file = "",
                                                              bool enable_console = true) {
  return std::make_shared<LoggerMiddleware>(name, level, format, log_file, enable_console);
}

inline std::shared_ptr<Middleware> create_cors_middleware(
    const std::string &origin = "*", const std::string &methods = "GET, POST, PUT, DELETE, OPTIONS",
    const std::string &headers = "Content-Type, Authorization") {
  return std::make_shared<CorsMiddleware>(origin, methods, headers);
}

inline std::shared_ptr<Middleware> create_response_time_middleware(const std::string &header_name = "X-Response-Time") {
  return std::make_shared<ResponseTimeMiddleware>(header_name);
}

inline std::shared_ptr<Middleware> create_simple_logger(const std::string &name = "SimpleLogger") {
  return create_logger_middleware(name, LogLevel::Info, LogFormat::Simple);
}

inline std::shared_ptr<Middleware> create_json_logger(const std::string &name = "JsonLogger",
                                                      const std::string &log_file = "app.json.log") {
  return create_logger_middleware(name, LogLevel::Info, LogFormat::Json, log_file);
}

inline std::shared_ptr<Middleware> create_access_logger(const std::string &name = "AccessLogger",
                                                        const std::string &log_file = "access.log") {
  return create_logger_middleware(name, LogLevel::Info, LogFormat::Apache, log_file);
}

inline std::shared_ptr<Middleware> create_debug_logger(const std::string &name = "DebugLogger",
                                                       const std::string &log_file = "debug.log") {
  return create_logger_middleware(name, LogLevel::Debug, LogFormat::Detailed, log_file, true);
}

inline std::shared_ptr<Middleware> create_performance_logger(const std::string &name = "PerformanceLogger",
                                                             const std::string &log_file = "performance.log") {
  return create_logger_middleware(name, LogLevel::Warn, LogFormat::Simple, log_file, false);
}

inline std::shared_ptr<Middleware> create_static_middleware(const std::string &url_prefix,
                                                            const std::filesystem::path &document_root,
                                                            const std::string &index_file = "index.html") {
  return std::make_shared<StaticMiddleware>(url_prefix, document_root, index_file);
}

inline std::shared_ptr<Middleware> create_body_parser_middleware(const std::string &name = "BodyParserMiddleware",
                                                                 bool bad_request_on_parse_error = true) {
  return std::make_shared<BodyParserMiddleware>(name, bad_request_on_parse_error);
}

}  // namespace factories

/// @deprecated Prefer @c foxhttp::middleware::factories — same functions, shorter namespace.
namespace middleware_factories {
using factories::create_access_logger;
using factories::create_body_parser_middleware;
using factories::create_cors_middleware;
using factories::create_debug_logger;
using factories::create_json_logger;
using factories::create_logger_middleware;
using factories::create_performance_logger;
using factories::create_response_time_middleware;
using factories::create_simple_logger;
using factories::create_static_middleware;
}  // namespace middleware_factories

}  // namespace foxhttp::middleware
