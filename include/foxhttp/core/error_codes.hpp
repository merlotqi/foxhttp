#pragma once

#include <string>

namespace foxhttp::core {

enum class ErrorCode {
  Success = 0,

  NetworkTimeout = 1000,
  NetworkConnectionRefused = 1001,
  NetworkConnectionReset = 1002,
  NetworkUnreachable = 1003,
  NetworkDnsResolutionFailed = 1004,

  ParseInvalidFormat = 2000,
  ParseSizeExceeded = 2001,
  ParseEncodingError = 2002,
  ParseJsonInvalid = 2003,
  ParseMultipartInvalid = 2004,

  MiddlewareTimeout = 3000,
  MiddlewareError = 3001,
  MiddlewareCancelled = 3002,
  MiddlewareChainExhausted = 3003,

  RouteNotFound = 4000,
  RouteInvalidPattern = 4001,
  RouteParameterMissing = 4002,

  ServerInternalError = 5000,
  ServerShutdown = 5001,
  ServerAcceptFailed = 5002,
  ServerSessionError = 5003,

  TlsHandshakeFailed = 6000,
  TlsCertificateInvalid = 6001,
  TlsCertificateExpired = 6002,

  WebsocketHandshakeFailed = 7000,
  WebsocketConnectionClosed = 7001,
  WebsocketMessageTooLarge = 7002,
  WebsocketProtocolError = 7003,

  ClientRequestFailed = 8000,
  ClientResponseInvalid = 8001,
  ClientConnectionPoolExhausted = 8002,

  Unknown = 9999
};

struct ErrorInfo {
  ErrorCode code;
  std::string message;
  std::string details;

  ErrorInfo(ErrorCode c, const std::string& msg, const std::string& det = "") : code(c), message(msg), details(det) {}
};

class MiddlewareException : public std::exception {
 public:
  MiddlewareException(const std::string& msg, ErrorCode code) : message_(msg), code_(code) {}

  const char* what() const noexcept override { return message_.c_str(); }
  ErrorCode code() const { return code_; }

 private:
  std::string message_;
  ErrorCode code_;
};

class RouterException : public std::exception {
 public:
  RouterException(const std::string& msg, ErrorCode code) : message_(msg), code_(code) {}

  const char* what() const noexcept override { return message_.c_str(); }
  ErrorCode code() const { return code_; }

 private:
  std::string message_;
  ErrorCode code_;
};

class ParserException : public std::exception {
 public:
  ParserException(const std::string& msg, ErrorCode code) : message_(msg), code_(code) {}

  const char* what() const noexcept override { return message_.c_str(); }
  ErrorCode code() const { return code_; }

 private:
  std::string message_;
  ErrorCode code_;
};

class NetworkException : public std::exception {
 public:
  NetworkException(const std::string& msg, ErrorCode code) : message_(msg), code_(code) {}

  const char* what() const noexcept override { return message_.c_str(); }
  ErrorCode code() const { return code_; }

 private:
  std::string message_;
  ErrorCode code_;
};

inline std::string error_code_to_string(ErrorCode code) {
  switch (code) {
    case ErrorCode::Success:
      return "Success";
    case ErrorCode::NetworkTimeout:
      return "Network timeout";
    case ErrorCode::NetworkConnectionRefused:
      return "Connection refused";
    case ErrorCode::NetworkConnectionReset:
      return "Connection reset";
    case ErrorCode::NetworkUnreachable:
      return "Network unreachable";
    case ErrorCode::NetworkDnsResolutionFailed:
      return "DNS resolution failed";
    case ErrorCode::ParseInvalidFormat:
      return "Invalid format";
    case ErrorCode::ParseSizeExceeded:
      return "Size exceeded";
    case ErrorCode::ParseEncodingError:
      return "Encoding error";
    case ErrorCode::ParseJsonInvalid:
      return "Invalid JSON";
    case ErrorCode::ParseMultipartInvalid:
      return "Invalid multipart";
    case ErrorCode::MiddlewareTimeout:
      return "Middleware timeout";
    case ErrorCode::MiddlewareError:
      return "Middleware error";
    case ErrorCode::MiddlewareCancelled:
      return "Middleware cancelled";
    case ErrorCode::MiddlewareChainExhausted:
      return "Middleware chain exhausted";
    case ErrorCode::RouteNotFound:
      return "Route not found";
    case ErrorCode::RouteInvalidPattern:
      return "Invalid route pattern";
    case ErrorCode::RouteParameterMissing:
      return "Route parameter missing";
    case ErrorCode::ServerInternalError:
      return "Internal server error";
    case ErrorCode::ServerShutdown:
      return "Server shutdown";
    case ErrorCode::ServerAcceptFailed:
      return "Accept failed";
    case ErrorCode::ServerSessionError:
      return "Session error";
    case ErrorCode::TlsHandshakeFailed:
      return "TLS handshake failed";
    case ErrorCode::TlsCertificateInvalid:
      return "Invalid certificate";
    case ErrorCode::TlsCertificateExpired:
      return "Certificate expired";
    case ErrorCode::WebsocketHandshakeFailed:
      return "WebSocket handshake failed";
    case ErrorCode::WebsocketConnectionClosed:
      return "WebSocket Connection closed";
    case ErrorCode::WebsocketMessageTooLarge:
      return "WebSocket message too large";
    case ErrorCode::WebsocketProtocolError:
      return "WebSocket protocol error";
    case ErrorCode::ClientRequestFailed:
      return "Client request failed";
    case ErrorCode::ClientResponseInvalid:
      return "Invalid response";
    case ErrorCode::ClientConnectionPoolExhausted:
      return "Connection pool exhausted";
    case ErrorCode::Unknown:
    default:
      return "Unknown error";
  }
}

}  // namespace foxhttp::core
