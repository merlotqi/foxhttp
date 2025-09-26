/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * middleware factory functions
 */

#pragma once

#include <foxhttp/middleware/basic/cors_middleware.hpp>
#include <foxhttp/middleware/basic/logger_middleware.hpp>
#include <foxhttp/middleware/basic/response_time_middleware.hpp>
#include <memory>

namespace foxhttp {
namespace middleware_factories {

inline std::shared_ptr<middleware> create_logger_middleware(const std::string &name = "LoggerMiddleware",
                                                            log_level level = log_level::info,
                                                            log_format format = log_format::detailed,
                                                            const std::string &log_file = "",
                                                            bool enable_console = true)
{
    return std::make_shared<logger_middleware>(name, level, format, log_file, enable_console);
}

inline std::shared_ptr<middleware>
create_cors_middleware(const std::string &origin = "*", const std::string &methods = "GET, POST, PUT, DELETE, OPTIONS",
                       const std::string &headers = "Content-Type, Authorization")
{
    return std::make_shared<cors_middleware>(origin, methods, headers);
}

inline std::shared_ptr<middleware> create_response_time_middleware(const std::string &header_name = "X-Response-Time")
{
    return std::make_shared<response_time_middleware>(header_name);
}

inline std::shared_ptr<middleware> create_simple_logger(const std::string &name = "SimpleLogger")
{
    return create_logger_middleware(name, log_level::info, log_format::simple);
}

inline std::shared_ptr<middleware> create_json_logger(const std::string &name = "JsonLogger",
                                                      const std::string &log_file = "app.json.log")
{
    return create_logger_middleware(name, log_level::info, log_format::json, log_file);
}

inline std::shared_ptr<middleware> create_access_logger(const std::string &name = "AccessLogger",
                                                        const std::string &log_file = "access.log")
{
    return create_logger_middleware(name, log_level::info, log_format::apache, log_file);
}

inline std::shared_ptr<middleware> create_debug_logger(const std::string &name = "DebugLogger",
                                                       const std::string &log_file = "debug.log")
{
    return create_logger_middleware(name, log_level::debug, log_format::detailed, log_file, true);
}

inline std::shared_ptr<middleware> create_performance_logger(const std::string &name = "PerformanceLogger",
                                                             const std::string &log_file = "performance.log")
{
    return create_logger_middleware(name, log_level::warn, log_format::simple, log_file, false);
}

}// namespace middleware_factories
}// namespace foxhttp
