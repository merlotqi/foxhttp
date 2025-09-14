/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * Middleware factory functions
 */

#pragma once

#include <foxhttp/middleware/basic/logger_middleware.hpp>
#include <foxhttp/middleware/basic/cors_middleware.hpp>
#include <foxhttp/middleware/basic/response_time_middleware.hpp>
#include <memory>

namespace foxhttp {

namespace middleware_factories {

// Create a logger middleware
inline std::shared_ptr<Middleware> create_logger_middleware(
    const std::string& name = "LoggerMiddleware",
    LogLevel level = LogLevel::INFO,
    LogFormat format = LogFormat::DETAILED,
    const std::string& log_file = "",
    bool enable_console = true) {
    return std::make_shared<LoggerMiddleware>(name, level, format, log_file, enable_console);
}

// Create a CORS middleware
inline std::shared_ptr<Middleware> create_cors_middleware(
    const std::string& origin = "*",
    const std::string& methods = "GET, POST, PUT, DELETE, OPTIONS",
    const std::string& headers = "Content-Type, Authorization") {
    return std::make_shared<CorsMiddleware>(origin, methods, headers);
}

// Create a response time middleware
inline std::shared_ptr<Middleware> create_response_time_middleware(
    const std::string& header_name = "X-Response-Time") {
    return std::make_shared<ResponseTimeMiddleware>(header_name);
}

// Convenience functions for common configurations

// Simple console logger
inline std::shared_ptr<Middleware> create_simple_logger(const std::string& name = "SimpleLogger") {
    return create_logger_middleware(name, LogLevel::INFO, LogFormat::SIMPLE);
}

// JSON logger for log aggregation
inline std::shared_ptr<Middleware> create_json_logger(
    const std::string& name = "JsonLogger",
    const std::string& log_file = "app.json.log") {
    return create_logger_middleware(name, LogLevel::INFO, LogFormat::JSON, log_file);
}

// Apache access logger
inline std::shared_ptr<Middleware> create_access_logger(
    const std::string& name = "AccessLogger",
    const std::string& log_file = "access.log") {
    return create_logger_middleware(name, LogLevel::INFO, LogFormat::APACHE, log_file);
}

// Debug logger for development
inline std::shared_ptr<Middleware> create_debug_logger(
    const std::string& name = "DebugLogger",
    const std::string& log_file = "debug.log") {
    return create_logger_middleware(name, LogLevel::DEBUG, LogFormat::DETAILED, log_file, true);
}

// Performance logger (warnings and errors only)
inline std::shared_ptr<Middleware> create_performance_logger(
    const std::string& name = "PerformanceLogger",
    const std::string& log_file = "performance.log") {
    return create_logger_middleware(name, LogLevel::WARN, LogFormat::SIMPLE, log_file, false);
}

} // namespace middleware_factories

} // namespace foxhttp
