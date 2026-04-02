#!/usr/bin/env python3
"""Canonical bulk refactor for foxhttp: namespaces, detail, PascalCase types, guarded substitutions.

Prefer this over scripts/apply_naming_refactor.py (deprecated).
"""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TEXT_EXTS = {".hpp", ".cpp", ".h", ".cc", ".cxx"}

SERVER_PROTECT = [
    ("@@F_SERVER1@@", "beast::role_type::server"),
    ("@@F_SERVER2@@", "asio::ssl::stream_base::server"),
    ("@@F_SERVER3@@", "http::field::server"),
]
EC_PROTECT = [
    ("@@EC1@@", "beast::error_code"),
    ("@@EC2@@", "std::error_code"),
    ("@@EC3@@", "boost::system::error_code"),
]

INCLUDE_GUARD = ("@@INC@@", r'(#include\s*[<"]foxhttp/[^>"]+[>"])')

SUBS: list[tuple[str, str]] = [
    ("multipart_stream_parser_core", "MultipartStreamParserImpl"),
    ("multipart_stream_parser", "MultipartStreamParser"),
    ("multipart_parser_core", "MultipartParserImpl"),
    ("multipart_field_core", "MultipartFieldImpl"),
    ("form_parser_core", "FormParserImpl"),
    ("form_field_core", "FormFieldImpl"),
    ("json_parser_core", "JsonParserImpl"),
    ("plain_text_parser_core", "PlainTextParserImpl"),
    ("multipart_parser", "MultipartParser"),
    ("multipart_field", "MultipartField"),
    ("io_context_pool", "IoContextPool"),
    ("connection_pool", "ConnectionPool"),
    ("request_context", "RequestContext"),
    ("middleware_chain", "MiddlewareChain"),
    ("middleware_priority", "MiddlewarePriority"),
    ("middleware_result", "MiddlewareResult"),
    ("middleware_builder", "MiddlewareBuilder"),
    ("middleware_stats", "MiddlewareStats"),
    ("middleware_exception", "MiddlewareException"),
    ("functional_middleware", "FunctionalMiddleware"),
    ("response_time_middleware", "ResponseTimeMiddleware"),
    ("compression_middleware", "CompressionMiddleware"),
    ("security_headers_middleware", "SecurityHeadersMiddleware"),
    ("body_parser_middleware", "BodyParserMiddleware"),
    ("logger_middleware", "LoggerMiddleware"),
    ("static_middleware", "StaticMiddleware"),
    ("cors_middleware", "CorsMiddleware"),
    ("rate_limit_middleware", "RateLimitMiddleware"),
    ("auth_middleware", "AuthMiddleware"),
    ("priority_middleware", "PriorityMiddleware"),
    ("conditional_middleware", "ConditionalMiddleware"),
    ("pipeline_execution_state", "PipelineExecutionState"),
    ("ssl_session", "SslSession"),
    ("ssl_server", "SslServer"),
    ("wss_session", "WssSession"),
    ("ws_session", "WsSession"),
    ("session_base", "SessionBase"),
    ("session_limits", "SessionLimits"),
    ("strand_pool_config", "StrandPoolConfig"),
    ("timer_manager_config", "TimerManagerConfig"),
    ("websocket_limits", "WebSocketLimits"),
    ("load_balance_strategy", "LoadBalanceStrategy"),
    ("plain_text_config", "PlainTextConfig"),
    ("multipart_config", "MultipartConfig"),
    ("json_config", "JsonConfig"),
    ("form_config", "FormConfig"),
    ("config_manager", "ConfigManager"),
    ("config_watcher", "ConfigWatcher"),
    ("json_parser", "JsonParser"),
    ("form_parser", "FormParser"),
    ("form_field", "FormField"),
    ("parser_factory", "ParserFactory"),
    ("parser_holder_base", "ParserHolderBase"),
    ("callable_api_handler", "CallableApiHandler"),
    ("api_handler", "ApiHandler"),
    ("route_table", "RouteTable"),
    ("request_builder", "RequestBuilder"),
    ("http_client", "HttpClient"),
    ("client_options", "ClientOptions"),
    ("request_timeout_options", "RequestTimeoutOptions"),
    ("strand_pool", "StrandPool"),
    ("timer_manager", "TimerManager"),
    ("signal_set", "SignalSet"),
    ("error_info", "ErrorInfo"),
    ("router_exception", "RouterException"),
    ("parser_exception", "ParserException"),
    ("network_exception", "NetworkException"),
    ("read_abort_reason", "ReadAbortReason"),
    ("compression_type", "CompressionType"),
    ("serve_result", "ServeResult"),
    ("auth_user", "AuthUser"),
    ("auth_scheme", "AuthScheme"),
    ("static_route", "StaticRoute"),
    ("dynamic_route", "DynamicRoute"),
    ("client_base_config", "ClientBaseConfig"),
    ("request_spec", "RequestSpec"),
    ("connection", "Connection"),
    ("timer_guard", "TimerGuard"),
]

ENUM_ERROR = [
    ("error_code::network_connection_refused", "ErrorCode::NetworkConnectionRefused"),
    ("error_code::network_connection_reset", "ErrorCode::NetworkConnectionReset"),
    ("error_code::network_dns_resolution_failed", "ErrorCode::NetworkDnsResolutionFailed"),
    ("error_code::parse_invalid_format", "ErrorCode::ParseInvalidFormat"),
    ("error_code::parse_size_exceeded", "ErrorCode::ParseSizeExceeded"),
    ("error_code::parse_encoding_error", "ErrorCode::ParseEncodingError"),
    ("error_code::parse_json_invalid", "ErrorCode::ParseJsonInvalid"),
    ("error_code::parse_multipart_invalid", "ErrorCode::ParseMultipartInvalid"),
    ("error_code::middleware_chain_exhausted", "ErrorCode::MiddlewareChainExhausted"),
    ("error_code::route_invalid_pattern", "ErrorCode::RouteInvalidPattern"),
    ("error_code::route_parameter_missing", "ErrorCode::RouteParameterMissing"),
    ("error_code::server_internal_error", "ErrorCode::ServerInternalError"),
    ("error_code::server_accept_failed", "ErrorCode::ServerAcceptFailed"),
    ("error_code::server_session_error", "ErrorCode::ServerSessionError"),
    ("error_code::tls_handshake_failed", "ErrorCode::TlsHandshakeFailed"),
    ("error_code::tls_certificate_invalid", "ErrorCode::TlsCertificateInvalid"),
    ("error_code::tls_certificate_expired", "ErrorCode::TlsCertificateExpired"),
    ("error_code::websocket_handshake_failed", "ErrorCode::WebsocketHandshakeFailed"),
    ("error_code::websocket_connection_closed", "ErrorCode::WebsocketConnectionClosed"),
    ("error_code::websocket_message_too_large", "ErrorCode::WebsocketMessageTooLarge"),
    ("error_code::websocket_protocol_error", "ErrorCode::WebsocketProtocolError"),
    ("error_code::client_request_failed", "ErrorCode::ClientRequestFailed"),
    ("error_code::client_response_invalid", "ErrorCode::ClientResponseInvalid"),
    ("error_code::client_connection_pool_exhausted", "ErrorCode::ClientConnectionPoolExhausted"),
    ("error_code::network_timeout", "ErrorCode::NetworkTimeout"),
    ("error_code::network_unreachable", "ErrorCode::NetworkUnreachable"),
    ("error_code::middleware_timeout", "ErrorCode::MiddlewareTimeout"),
    ("error_code::middleware_cancelled", "ErrorCode::MiddlewareCancelled"),
    ("error_code::middleware_error", "ErrorCode::MiddlewareError"),
    ("error_code::route_not_found", "ErrorCode::RouteNotFound"),
    ("error_code::server_shutdown", "ErrorCode::ServerShutdown"),
    ("error_code::success", "ErrorCode::Success"),
    ("error_code::unknown", "ErrorCode::Unknown"),
]

ENUM_MISC = [
    ("middleware_priority::lowest", "MiddlewarePriority::Lowest"),
    ("middleware_priority::highest", "MiddlewarePriority::Highest"),
    ("middleware_priority::normal", "MiddlewarePriority::Normal"),
    ("middleware_priority::low", "MiddlewarePriority::Low"),
    ("middleware_priority::high", "MiddlewarePriority::High"),
    ("middleware_result::continue_", "MiddlewareResult::Continue"),
    ("middleware_result::stop", "MiddlewareResult::Stop"),
    ("middleware_result::error", "MiddlewareResult::Error"),
    ("middleware_result::timeout", "MiddlewareResult::Timeout"),
    ("load_balance_strategy::weighted_round_robin", "LoadBalanceStrategy::WeightedRoundRobin"),
    ("load_balance_strategy::round_robin", "LoadBalanceStrategy::RoundRobin"),
    ("load_balance_strategy::least_connections", "LoadBalanceStrategy::LeastConnections"),
    ("load_balance_strategy::consistent_hash", "LoadBalanceStrategy::ConsistentHash"),
    ("load_balance_strategy::random", "LoadBalanceStrategy::Random"),
    ("read_abort_reason::header_timeout", "ReadAbortReason::HeaderTimeout"),
    ("read_abort_reason::body_timeout", "ReadAbortReason::BodyTimeout"),
    ("read_abort_reason::none", "ReadAbortReason::None"),
    ("log_level::critical", "LogLevel::Critical"),
    ("log_level::trace", "LogLevel::Trace"),
    ("log_level::debug", "LogLevel::Debug"),
    ("log_level::info", "LogLevel::Info"),
    ("log_level::warn", "LogLevel::Warn"),
    ("log_level::error", "LogLevel::Error"),
    ("log_format::detailed", "LogFormat::Detailed"),
    ("log_format::simple", "LogFormat::Simple"),
    ("log_format::json", "LogFormat::Json"),
    ("log_format::apache", "LogFormat::Apache"),
    ("auth_scheme::bearer_jwt", "AuthScheme::BearerJwt"),
    ("auth_scheme::basic", "AuthScheme::Basic"),
    ("auth_scheme::digest", "AuthScheme::Digest"),
    ("auth_scheme::api_key", "AuthScheme::ApiKey"),
    ("auth_scheme::custom", "AuthScheme::Custom"),
]

NS_DETAILS = [
    ("namespace details {", "namespace detail {"),
    ("}  // namespace details", "}  // namespace detail"),
    ("details::", "detail::"),
]

FOX_QUAL: list[tuple[str, str]] = sorted(
    [
        ("::foxhttp::plain_text_config", "::foxhttp::config::PlainTextConfig"),
        ("::foxhttp::multipart_config", "::foxhttp::config::MultipartConfig"),
        ("::foxhttp::json_config", "::foxhttp::config::JsonConfig"),
        ("::foxhttp::form_config", "::foxhttp::config::FormConfig"),
        ("::foxhttp::strand_pool_config", "::foxhttp::config::StrandPoolConfig"),
        ("::foxhttp::timer_manager_config", "::foxhttp::config::TimerManagerConfig"),
        ("foxhttp::client::http_client", "foxhttp::client::HttpClient"),
        ("foxhttp::client::request_builder", "foxhttp::client::RequestBuilder"),
        ("foxhttp::detail::timer_guard", "foxhttp::detail::TimerGuard"),
        ("foxhttp::middleware_result::", "foxhttp::middleware::MiddlewareResult::"),
        ("foxhttp::middleware_priority::", "foxhttp::middleware::MiddlewarePriority::"),
        ("foxhttp::middleware_chain", "foxhttp::middleware::MiddlewareChain"),
        ("foxhttp::middleware_builder", "foxhttp::middleware::MiddlewareBuilder"),
        ("foxhttp::functional_middleware", "foxhttp::middleware::FunctionalMiddleware"),
        ("foxhttp::response_time_middleware", "foxhttp::middleware::ResponseTimeMiddleware"),
        ("foxhttp::compression_middleware", "foxhttp::middleware::CompressionMiddleware"),
        ("foxhttp::security_headers_middleware", "foxhttp::middleware::SecurityHeadersMiddleware"),
        ("foxhttp::body_parser_middleware", "foxhttp::middleware::BodyParserMiddleware"),
        ("foxhttp::logger_middleware", "foxhttp::middleware::LoggerMiddleware"),
        ("foxhttp::static_middleware", "foxhttp::middleware::StaticMiddleware"),
        ("foxhttp::cors_middleware", "foxhttp::middleware::CorsMiddleware"),
        ("foxhttp::rate_limit_middleware", "foxhttp::middleware::RateLimitMiddleware"),
        ("foxhttp::auth_middleware", "foxhttp::middleware::AuthMiddleware"),
        ("foxhttp::plain_text_parser", "foxhttp::parser::PlainTextParser"),
        ("foxhttp::multipart_stream_parser", "foxhttp::parser::MultipartStreamParser"),
        ("foxhttp::multipart_parser", "foxhttp::parser::MultipartParser"),
        ("foxhttp::form_parser", "foxhttp::parser::FormParser"),
        ("foxhttp::json_parser", "foxhttp::parser::JsonParser"),
        ("foxhttp::route_table", "foxhttp::router::RouteTable"),
        ("foxhttp::router::register_static_handler", "foxhttp::router::Router::register_static_handler"),
        ("foxhttp::router::register_dynamic_handler", "foxhttp::router::Router::register_dynamic_handler"),
        ("foxhttp::router::resolve_route", "foxhttp::router::Router::resolve_route"),
        ("foxhttp::request_context", "foxhttp::server::RequestContext"),
        ("foxhttp::io_context_pool", "foxhttp::server::IoContextPool"),
        ("foxhttp::ssl_server", "foxhttp::server::SslServer"),
        ("foxhttp::ssl_session", "foxhttp::server::SslSession"),
        ("foxhttp::session_base", "foxhttp::server::SessionBase"),
        ("foxhttp::session", "foxhttp::server::Session"),
        ("foxhttp::server server", "foxhttp::server::Server server"),
    ],
    key=lambda x: -len(x[0]),
)

TYPE_FIXES = [
    (r"\bclass route\b", "class Route"),
    (r"\bpublic route\b", "public Route"),
    (r"std::shared_ptr<route>", "std::shared_ptr<Route>"),
    (r"std::vector<std::shared_ptr<route>>", "std::vector<std::shared_ptr<Route>>"),
    (r"\bclass router\b", "class Router"),
    (r"\bclass server\b", "class Server"),
    (r"\bclass session\b", "class Session"),
    (r"\bclass middleware\b", "class Middleware"),
    (r"\benable_shared_from_this<session>", "enable_shared_from_this<Session>"),
    (r"std::shared_ptr<session>", "std::shared_ptr<Session>"),
    (r"std::shared_ptr<server>", "std::shared_ptr<Server>"),
    (r"std::shared_ptr<middleware>", "std::shared_ptr<Middleware>"),
    (r"std::shared_ptr<router>", "std::shared_ptr<Router>"),
    (r"\bserver::", "Server::"),
    (r"\bsession::", "Session::"),
    (r"\bmiddleware::", "Middleware::"),
    (r"\brouter::", "Router::"),
    (r"\broute::", "Route::"),
    (r"\bstatic_route::", "StaticRoute::"),
    (r"\bdynamic_route::", "DynamicRoute::"),
    (r"\broute_table::", "RouteTable::"),
]


def shield_includes(text: str) -> tuple[str, list[str]]:
    lines = text.splitlines(keepends=True)
    out: list[str] = []
    vault: list[str] = []
    for line in lines:
        if line.lstrip().startswith("#include") and "foxhttp/" in line:
            vault.append(line)
            out.append(f"__FOX_INC_{len(vault)-1}__\n")
        else:
            out.append(line)
    return "".join(out), vault


def unshield_includes(text: str, vault: list[str]) -> str:
    for i, inc in enumerate(vault):
        text = text.replace(f"__FOX_INC_{i}__", inc.rstrip("\n"))
    return text


def apply_subs(text: str) -> str:
    for tok, orig in SERVER_PROTECT + EC_PROTECT:
        text = text.replace(orig, tok)
    text, inc_vault = shield_includes(text)
    for old, new in ENUM_ERROR:
        text = text.replace(old, new)
    for old, new in ENUM_MISC:
        text = text.replace(old, new)
    for old, new in NS_DETAILS:
        text = text.replace(old, new)
    for old, new in SUBS:
        text = re.sub(rf"\b{re.escape(old)}\b", new, text)
    for pat, rep in TYPE_FIXES:
        text = re.sub(pat, rep, text)
    text = re.sub(r"\bparser<", "Parser<", text)
    text = re.sub(r"\bclass parser\b", "class Parser", text)
    for token, orig in SERVER_PROTECT:
        text = text.replace(token, orig)
    text = re.sub(r"\berror_code\b", "ErrorCode", text)
    for token, orig in EC_PROTECT:
        text = text.replace(token, orig)
    for old, new in FOX_QUAL:
        text = text.replace(old, new)
    text = unshield_includes(text, inc_vault)
    return text


def wrap_outer_namespace(rel: str, text: str) -> str:
    rel_posix = rel.replace("\\", "/")
    if "/foxhttp/config/detail/" in rel_posix or rel_posix.endswith("config/detail/config_watcher.hpp"):
        return text.replace("namespace foxhttp {", "namespace foxhttp::config::detail {", 1).replace(
            "}  // namespace foxhttp", "}  // namespace foxhttp::config::detail", 1
        )
    if "/foxhttp/config/" in rel_posix and rel_posix.startswith("include/"):
        if rel_posix.endswith("include/foxhttp/config.hpp"):
            return text
        return text.replace("namespace foxhttp {", "namespace foxhttp::config {", 1).replace(
            "}  // namespace foxhttp", "}  // namespace foxhttp::config", 1
        )
    if "src/config/" in rel_posix:
        if "/detail/" in rel_posix:
            return text.replace("namespace foxhttp {", "namespace foxhttp::config::detail {", 1).replace(
                "}  // namespace foxhttp", "}  // namespace foxhttp::config::detail", 1
            )
        return text.replace("namespace foxhttp {", "namespace foxhttp::config {", 1).replace(
            "}  // namespace foxhttp", "}  // namespace foxhttp::config", 1
        )
    if "/foxhttp/parser/detail/" in rel_posix:
        return text.replace("namespace foxhttp {", "namespace foxhttp::parser::detail {", 1).replace(
            "}  // namespace foxhttp", "}  // namespace foxhttp::parser::detail", 1
        )
    if "/foxhttp/parser/" in rel_posix and rel_posix.startswith("include/"):
        return text.replace("namespace foxhttp {", "namespace foxhttp::parser {", 1).replace(
            "}  // namespace foxhttp", "}  // namespace foxhttp::parser", 1
        )
    if "src/parser/" in rel_posix:
        if "namespace foxhttp {" in text:
            text = text.replace("namespace foxhttp {", "namespace foxhttp::parser {", 1)
            text = text.replace("}  // namespace foxhttp", "}  // namespace foxhttp::parser", 1)
        return text
    if rel_posix.endswith("include/foxhttp/middleware/middleware_factories.hpp"):
        text = text.replace(
            "namespace foxhttp {\nnamespace middleware_factories {",
            "namespace foxhttp::middleware::middleware_factories {",
            1,
        )
        text = text.replace(
            "}  // namespace middleware_factories\n}  // namespace foxhttp",
            "}  // namespace foxhttp::middleware::middleware_factories",
            1,
        )
        return text
    if "/foxhttp/middleware/advanced/" in rel_posix:
        text = text.replace(
            "namespace foxhttp {\nnamespace advanced {",
            "namespace foxhttp::middleware::advanced {",
            1,
        )
        text = text.replace(
            "}  // namespace advanced\n}  // namespace foxhttp",
            "}  // namespace foxhttp::middleware::advanced",
            1,
        )
        return text
    if "/foxhttp/middleware/" in rel_posix and rel_posix.startswith("include/"):
        return text.replace("namespace foxhttp {", "namespace foxhttp::middleware {", 1).replace(
            "}  // namespace foxhttp", "}  // namespace foxhttp::middleware", 1
        )
    if "src/middleware/" in rel_posix:
        if "/advanced/" in rel_posix:
            text = text.replace(
                "namespace foxhttp {\nnamespace advanced {",
                "namespace foxhttp::middleware::advanced {",
                1,
            )
            text = text.replace(
                "}  // namespace advanced\n}  // namespace foxhttp",
                "}  // namespace foxhttp::middleware::advanced",
                1,
            )
            return text
        return text.replace("namespace foxhttp {", "namespace foxhttp::middleware {", 1).replace(
            "}  // namespace foxhttp", "}  // namespace foxhttp::middleware", 1
        )
    if "/foxhttp/server/" in rel_posix and rel_posix.startswith("include/"):
        return text.replace("namespace foxhttp {", "namespace foxhttp::server {", 1).replace(
            "}  // namespace foxhttp", "}  // namespace foxhttp::server", 1
        )
    if "src/server/" in rel_posix:
        return text.replace("namespace foxhttp {", "namespace foxhttp::server {", 1).replace(
            "}  // namespace foxhttp", "}  // namespace foxhttp::server", 1
        )
    if "/foxhttp/router/" in rel_posix and rel_posix.startswith("include/"):
        return text.replace("namespace foxhttp {", "namespace foxhttp::router {", 1).replace(
            "}  // namespace foxhttp", "}  // namespace foxhttp::router", 1
        )
    if "src/router/" in rel_posix:
        return text.replace("namespace foxhttp {", "namespace foxhttp::router {", 1).replace(
            "}  // namespace foxhttp", "}  // namespace foxhttp::router", 1
        )
    if "/foxhttp/handler/" in rel_posix and rel_posix.startswith("include/"):
        return text.replace("namespace foxhttp {", "namespace foxhttp::handler {", 1).replace(
            "}  // namespace foxhttp", "}  // namespace foxhttp::handler", 1
        )
    if "/foxhttp/core/" in rel_posix and rel_posix.startswith("include/"):
        text = text.replace("namespace foxhttp::constants {", "namespace foxhttp::core {", 1)
        text = text.replace("}  // namespace foxhttp::constants", "}  // namespace foxhttp::core", 1)
        if "namespace foxhttp {" in text:
            text = text.replace("namespace foxhttp {", "namespace foxhttp::core {", 1)
            text = text.replace("}  // namespace foxhttp", "}  // namespace foxhttp::core", 1)
        return text
    return text


def process_file(path: Path) -> bool:
    rel = str(path.relative_to(ROOT))
    raw = path.read_text(encoding="utf-8")
    text = apply_subs(raw)
    if rel.startswith("include/") or rel.startswith("src/"):
        text = wrap_outer_namespace(rel, text)
    if text != raw:
        path.write_text(text, encoding="utf-8")
        return True
    return False


def main() -> None:
    roots = [ROOT / "include", ROOT / "src", ROOT / "tests", ROOT / "examples", ROOT / "benchmark"]
    n = 0
    for base in roots:
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.suffix not in TEXT_EXTS:
                continue
            if process_file(path):
                n += 1
                print(path.relative_to(ROOT))
    print("files updated:", n)


if __name__ == "__main__":
    main()
