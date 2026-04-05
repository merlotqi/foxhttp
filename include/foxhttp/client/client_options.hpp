#pragma once

#include <chrono>
#include <cstddef>
#include <foxhttp/core/constants.hpp>

namespace foxhttp::client {

/// Configuration options for HTTP client
struct ClientOptions {
  /// Connection timeout (time to establish TCP Connection)
  std::chrono::milliseconds connection_timeout{core::kDefaultIoTimeout};

  /// Request timeout (time to send request and receive response)
  std::chrono::milliseconds request_timeout{core::kDefaultIoTimeout};

  /// SSL handshake timeout (only applicable for HTTPS)
  std::chrono::milliseconds handshake_timeout{core::kDefaultHandshakeTimeout};

  /// Maximum response body size (0 = unlimited)
  std::size_t max_response_size{0};

  /// Whether to follow redirects (default: false for safety)
  bool follow_redirects{false};

  /// Maximum number of redirects to follow
  std::size_t max_redirects{5};

  /// User-Agent header value
  std::string user_agent;

  /// Set Connection timeout
  ClientOptions& set_connection_timeout(std::chrono::milliseconds timeout) {
    connection_timeout = timeout;
    return *this;
  }

  /// Set request timeout
  ClientOptions& set_request_timeout(std::chrono::milliseconds timeout) {
    request_timeout = timeout;
    return *this;
  }

  /// Set SSL handshake timeout
  ClientOptions& set_handshake_timeout(std::chrono::milliseconds timeout) {
    handshake_timeout = timeout;
    return *this;
  }

  /// Set maximum response size
  ClientOptions& set_max_response_size(std::size_t size) {
    max_response_size = size;
    return *this;
  }

  /// Enable or disable redirect following
  ClientOptions& set_follow_redirects(bool follow, std::size_t max = 5) {
    follow_redirects = follow;
    max_redirects = max;
    return *this;
  }

  /// Set User-Agent header
  ClientOptions& set_user_agent(std::string ua) {
    user_agent = std::move(ua);
    return *this;
  }
};

/// Per-request timeout overrides
struct RequestTimeoutOptions {
  /// Override Connection timeout for this request
  std::chrono::milliseconds connection_timeout{0};  // 0 = use client default

  /// Override request timeout for this request
  std::chrono::milliseconds request_timeout{0};  // 0 = use client default

  /// Override handshake timeout for this request
  std::chrono::milliseconds handshake_timeout{0};  // 0 = use client default

  /// Set Connection timeout for this request
  RequestTimeoutOptions& set_connection_timeout(std::chrono::milliseconds timeout) {
    connection_timeout = timeout;
    return *this;
  }

  /// Set request timeout for this request
  RequestTimeoutOptions& set_request_timeout(std::chrono::milliseconds timeout) {
    request_timeout = timeout;
    return *this;
  }

  /// Set handshake timeout for this request
  RequestTimeoutOptions& set_handshake_timeout(std::chrono::milliseconds timeout) {
    handshake_timeout = timeout;
    return *this;
  }
};

}  // namespace foxhttp::client
