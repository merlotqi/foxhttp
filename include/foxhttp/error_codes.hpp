#pragma once

#include <string>

namespace foxhttp {

enum class error_code {
  success = 0,

  network_timeout = 1000,
  network_connection_refused = 1001,
  network_connection_reset = 1002,
  network_unreachable = 1003,
  network_dns_resolution_failed = 1004,

  parse_invalid_format = 2000,
  parse_size_exceeded = 2001,
  parse_encoding_error = 2002,
  parse_json_invalid = 2003,
  parse_multipart_invalid = 2004,

  middleware_timeout = 3000,
  middleware_error = 3001,
  middleware_cancelled = 3002,
  middleware_chain_exhausted = 3003,

  route_not_found = 4000,
  route_invalid_pattern = 4001,
  route_parameter_missing = 4002,

  server_internal_error = 5000,
  server_shutdown = 5001,
  server_accept_failed = 5002,
  server_session_error = 5003,

  tls_handshake_failed = 6000,
  tls_certificate_invalid = 6001,
  tls_certificate_expired = 6002,

  websocket_handshake_failed = 7000,
  websocket_connection_closed = 7001,
  websocket_message_too_large = 7002,
  websocket_protocol_error = 7003,

  client_request_failed = 8000,
  client_response_invalid = 8001,
  client_connection_pool_exhausted = 8002,

  unknown = 9999
};

struct error_info {
  error_code code;
  std::string message;
  std::string details;

  error_info(error_code c, const std::string& msg, const std::string& det = "") : code(c), message(msg), details(det) {}
};

class middleware_exception : public std::exception {
 public:
  middleware_exception(const std::string& msg, error_code code) : message_(msg), code_(code) {}

  const char* what() const noexcept override { return message_.c_str(); }
  error_code code() const { return code_; }

 private:
  std::string message_;
  error_code code_;
};

class router_exception : public std::exception {
 public:
  router_exception(const std::string& msg, error_code code) : message_(msg), code_(code) {}

  const char* what() const noexcept override { return message_.c_str(); }
  error_code code() const { return code_; }

 private:
  std::string message_;
  error_code code_;
};

class parser_exception : public std::exception {
 public:
  parser_exception(const std::string& msg, error_code code) : message_(msg), code_(code) {}

  const char* what() const noexcept override { return message_.c_str(); }
  error_code code() const { return code_; }

 private:
  std::string message_;
  error_code code_;
};

class network_exception : public std::exception {
 public:
  network_exception(const std::string& msg, error_code code) : message_(msg), code_(code) {}

  const char* what() const noexcept override { return message_.c_str(); }
  error_code code() const { return code_; }

 private:
  std::string message_;
  error_code code_;
};

inline std::string error_code_to_string(error_code code) {
  switch (code) {
    case error_code::success:
      return "Success";
    case error_code::network_timeout:
      return "Network timeout";
    case error_code::network_connection_refused:
      return "Connection refused";
    case error_code::network_connection_reset:
      return "Connection reset";
    case error_code::network_unreachable:
      return "Network unreachable";
    case error_code::network_dns_resolution_failed:
      return "DNS resolution failed";
    case error_code::parse_invalid_format:
      return "Invalid format";
    case error_code::parse_size_exceeded:
      return "Size exceeded";
    case error_code::parse_encoding_error:
      return "Encoding error";
    case error_code::parse_json_invalid:
      return "Invalid JSON";
    case error_code::parse_multipart_invalid:
      return "Invalid multipart";
    case error_code::middleware_timeout:
      return "Middleware timeout";
    case error_code::middleware_error:
      return "Middleware error";
    case error_code::middleware_cancelled:
      return "Middleware cancelled";
    case error_code::middleware_chain_exhausted:
      return "Middleware chain exhausted";
    case error_code::route_not_found:
      return "Route not found";
    case error_code::route_invalid_pattern:
      return "Invalid route pattern";
    case error_code::route_parameter_missing:
      return "Route parameter missing";
    case error_code::server_internal_error:
      return "Internal server error";
    case error_code::server_shutdown:
      return "Server shutdown";
    case error_code::server_accept_failed:
      return "Accept failed";
    case error_code::server_session_error:
      return "Session error";
    case error_code::tls_handshake_failed:
      return "TLS handshake failed";
    case error_code::tls_certificate_invalid:
      return "Invalid certificate";
    case error_code::tls_certificate_expired:
      return "Certificate expired";
    case error_code::websocket_handshake_failed:
      return "WebSocket handshake failed";
    case error_code::websocket_connection_closed:
      return "WebSocket connection closed";
    case error_code::websocket_message_too_large:
      return "WebSocket message too large";
    case error_code::websocket_protocol_error:
      return "WebSocket protocol error";
    case error_code::client_request_failed:
      return "Client request failed";
    case error_code::client_response_invalid:
      return "Invalid response";
    case error_code::client_connection_pool_exhausted:
      return "Connection pool exhausted";
    case error_code::unknown:
    default:
      return "Unknown error";
  }
}

}  // namespace foxhttp
