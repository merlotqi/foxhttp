#pragma once

#include <boost/beast/http.hpp>
#include <functional>
#include <string>
#include <vector>

#if defined(USING_TLS)
#include <boost/asio/ssl/context.hpp>
#endif

namespace foxhttp::client::detail {

namespace http = boost::beast::http;

struct ClientBaseConfig {
  bool https{false};
  std::string host;
  std::string port;        // decimal string, e.g. "80"
  std::string host_field;  // Host header value (host:port when non-default)
};

struct RequestSpec {
  ClientBaseConfig base;
  http::verb method{http::verb::get};
  std::string target_path;  // path + query for request line
  std::vector<std::pair<std::string, std::string>> headers;
  std::string body;

#if defined(USING_TLS)
  boost::asio::ssl::context *ssl_ctx{nullptr};
#endif

  std::function<void(const http::response<http::string_body> &)> on_success;
  std::function<void(const boost::system::error_code &)> on_error;
};

}  // namespace foxhttp::client::detail
