#pragma once

#include <chrono>
#include <cstddef>
#include <foxhttp/constants.hpp>

namespace foxhttp::client {

/// Configuration options for HTTP client
struct client_options {
  /// Connection timeout (time to establish TCP connection)
  std::chrono::milliseconds connection_timeout{constants::kDefaultIoTimeout};

  /// Request timeout (time to send request and receive response)
  std::chrono::milliseconds request_timeout{constants::kDefaultIoTimeout};

  /// SSL handshake timeout (only applicable for HTTPS)
  std::chrono::milliseconds handshake_timeout{constants::kDefaultHandshakeTimeout};

  /// Maximum response body size (0 = unlimited)
  std::size_t max_response_size{0};

  /// Whether to follow redirects (default: false for safety)
  bool follow_redirects{false};

  /// Maximum number of redirects to follow
  std::size_t max_redirects{5};

  /// User-Agent header value
  std::string user_agent;

  /// Set connection timeout
  client_options& set_connection_timeout(std::chrono::milliseconds timeout) {
    connection_timeout = timeout;
    return *this;
  }

  /// Set request timeout
  client_options& set_request_timeout(std::chrono::milliseconds timeout) {
    request_timeout = timeout;
    return *this;
  }

  /// Set SSL handshake timeout
  client_options& set_handshake_timeout(std::chrono::milliseconds timeout) {
    handshake_timeout = timeout;
    return *this;
  }

  /// Set maximum response size
  client_options& set_max_response_size(std::size_t size) {
    max_response_size = size;
    return *this;
  }

  /// Enable or disable redirect following
  client_options& set_follow_redirects(bool follow, std::size_t max = 5) {
    follow_redirects = follow;
    max_redirects = max;
    return *this;
  }

  /// Set User-Agent header
  client_options& set_user_agent(std::string ua) {
    user_agent = std::move(ua);
    return *this;
  }
};

/// Per-request timeout overrides
struct request_timeout_options {
  /// Override connection timeout for this request
  std::chrono::milliseconds connection_timeout{0};  // 0 = use client default

  /// Override request timeout for this request
  std::chrono::milliseconds request_timeout{0};  // 0 = use client default

  /// Override handshake timeout for this request
  std::chrono::milliseconds handshake_timeout{0};  // 0 = use client default

  /// Set connection timeout for this request
  request_timeout_options& set_connection_timeout(std::chrono::milliseconds timeout) {
    connection_timeout = timeout;
    return *this;
  }

  /// Set request timeout for this request
  request_timeout_options& set_request_timeout(std::chrono::milliseconds timeout) {
    request_timeout = timeout;
    return *this;
  }

  /// Set handshake timeout for this request
  request_timeout_options& set_handshake_timeout(std::chrono::milliseconds timeout) {
    handshake_timeout = timeout;
    return *this;
  }
};

}  // namespace foxhttp::client
