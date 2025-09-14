/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

// Core components
#include <foxhttp/core/io_context_pool.hpp>
#include <foxhttp/core/request_context.hpp>
#include <foxhttp/core/server.hpp>
#include <foxhttp/core/session.hpp>
#include <foxhttp/core/signal_set.hpp>

// Middleware system
#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/middleware/middleware_factories.hpp>

// Basic middleware implementations
#include <foxhttp/middleware/basic/body_parser_middleware.hpp>
#include <foxhttp/middleware/basic/cors_middleware.hpp>
#include <foxhttp/middleware/basic/functional_middleware.hpp>
#include <foxhttp/middleware/basic/logger_middleware.hpp>
#include <foxhttp/middleware/basic/response_time_middleware.hpp>
#include <foxhttp/middleware/basic/static_middleware.hpp>

// Advanced middleware implementations
#include <foxhttp/middleware/advanced/auth_middleware.hpp>
#include <foxhttp/middleware/advanced/compression_middleware.hpp>
#include <foxhttp/middleware/advanced/rate_limit_middleware.hpp>

// Parser components
#include <foxhttp/parser/form_parser.hpp>
#include <foxhttp/parser/json_parser.hpp>
#include <foxhttp/parser/multipart_parser.hpp>
#include <foxhttp/parser/parser.hpp>
#include <foxhttp/parser/plaintext_parser.hpp>

// Router components
#include <foxhttp/handler/api_handler.hpp>
#include <foxhttp/router/route.hpp>
#include <foxhttp/router/route_table.hpp>
#include <foxhttp/router/router.hpp>

// Configuration
#include <foxhttp/cfg/config.hpp>
