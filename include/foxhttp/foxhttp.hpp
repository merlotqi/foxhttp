#pragma once

// Server components
#include <foxhttp/server/io_context_pool.hpp>
#include <foxhttp/server/request_context.hpp>
#include <foxhttp/server/server.hpp>
#include <foxhttp/server/session.hpp>
#include <foxhttp/server/signal_set.hpp>

// middleware system
#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/middleware/middleware_factories.hpp>

// Basic middleware implementations
#include <foxhttp/middleware/basic/cors_middleware.hpp>
#include <foxhttp/middleware/basic/functional_middleware.hpp>
#include <foxhttp/middleware/basic/logger_middleware.hpp>
#include <foxhttp/middleware/basic/response_time_middleware.hpp>

// Advanced middleware implementations
#include <foxhttp/middleware/advanced/auth_middleware.hpp>
#include <foxhttp/middleware/advanced/compression_middleware.hpp>
#include <foxhttp/middleware/advanced/rate_limit_middleware.hpp>
#include <foxhttp/middleware/advanced/security_headers_middleware.hpp>

// Parser components
#include <foxhttp/parser/form_parser.hpp>
#include <foxhttp/parser/json_parser.hpp>
#include <foxhttp/parser/multipart_parser.hpp>
#include <foxhttp/parser/parser.hpp>
#include <foxhttp/parser/plaintext_parser.hpp>

// Router components
#include <foxhttp/handler/api_handler.hpp>
#include <foxhttp/handler/callable_api_handler.hpp>
#include <foxhttp/router/route.hpp>
#include <foxhttp/router/route_table.hpp>
#include <foxhttp/router/router.hpp>

// Configuration
#include <foxhttp/config/config_manager.hpp>
#include <foxhttp/config/configs.hpp>

#ifdef USING_TLS
// TLS server components
#include <foxhttp/config/tls.hpp>
#include <foxhttp/server/ssl_server.hpp>
#include <foxhttp/server/ssl_session.hpp>
#endif

// WebSocket components
#include <foxhttp/server/ws_session.hpp>
#ifdef USING_TLS
#include <foxhttp/server/wss_session.hpp>
#endif
