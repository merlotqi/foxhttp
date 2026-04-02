#!/usr/bin/env python3
"""DEPRECATED — use scripts/full_refactor.py instead.

That script is the canonical bulk refactor (guarded substitutions, namespaces, detail).
This file is retained for historical reference only; running both may conflict or miss guards.
"""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# Longest keys first. Types / namespaces to new PascalCase (or Impl suffix for parser internals).
SUBSTITUTIONS: list[tuple[str, str]] = [
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
    ("plain_text_parser", "PlainTextParser"),
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
    ("error_code_to_string", "error_code_to_string"),  # keep snake_case function name
    ("middleware_factories", "MiddlewareFactories"),
    ("read_abort_reason", "ReadAbortReason"),
    ("compression_type", "CompressionType"),
    ("serve_result", "ServeResult"),
    ("auth_user", "AuthUser"),
    ("log_format", "LogFormat"),
    ("log_level", "LogLevel"),
    ("client_base_config", "ClientBaseConfig"),
    ("request_spec", "RequestSpec"),
    # error_code enum type -> ErrorCode (after error_code:: cases fixed separately)
    ("router", "Router"),
    ("route", "Route"),
    ("server", "Server"),
    ("session", "Session"),
]

# error_code::value -> ErrorCode::Value (enumerators)
ENUM_CASES = [
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

# Middleware / misc enum values (snake -> Pascal)
MIDDLEWARE_ENUM = [
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
]

NAMESPACE_FIXES = [
    ("namespace details {", "namespace detail {"),
    ("namespace details\n{", "namespace detail\n{"),
    ("}  // namespace details", "}  // namespace detail"),
    ("details::", "detail::"),
]

# template / class parser_holder — replace after parser_factory
PARSER_HOLDER_SUB = ("parser_holder", "ParserHolder")


def apply_to_content(text: str) -> str:
    for old, new in ENUM_CASES:
        text = text.replace(old, new)
    for old, new in MIDDLEWARE_ENUM:
        text = text.replace(old, new)
    for old, new in NAMESPACE_FIXES:
        text = text.replace(old, new)
    for old, new in SUBSTITUTIONS:
        text = re.sub(rf"\b{re.escape(old)}\b", new, text)
    old, new = PARSER_HOLDER_SUB
    text = re.sub(rf"\b{re.escape(old)}\b", new, text)
    # Remaining error_code as a type name (not ErrorCode::)
    text = re.sub(r"\berror_code\b", "ErrorCode", text)
    return text


def main() -> None:
    exts = {".hpp", ".cpp", ".md"}
    dirs = [ROOT / "include", ROOT / "src", ROOT / "tests", ROOT / "examples", ROOT / "benchmark"]
    for base in dirs:
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.suffix not in exts:
                continue
            if "scripts" in path.parts:
                continue
            raw = path.read_text(encoding="utf-8")
            new = apply_to_content(raw)
            if new != raw:
                path.write_text(new, encoding="utf-8")
                print("updated", path.relative_to(ROOT))


if __name__ == "__main__":
    main()
