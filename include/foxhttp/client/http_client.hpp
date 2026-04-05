#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/config.hpp>
#include <foxhttp/client/client_options.hpp>

#if FOXHTTP_HAS_COROUTINES
#include <boost/asio/awaitable.hpp>
#endif
#include <foxhttp/client/detail/request_spec.hpp>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

#if defined(USING_TLS)
#include <boost/asio/ssl/context.hpp>
#endif

namespace foxhttp::client {

namespace beast = boost::beast;
namespace http = beast::http;

/// Fluent HTTP/HTTPS client (Promise-style \ref RequestBuilder::then / \ref RequestBuilder::catch_error,
/// or \ref RequestBuilder::as_awaitable for coroutines when C++20 is enabled).
/// One-shot per builder: do not mix \c then() and \c as_awaitable().
class RequestBuilder;

/// HTTP(S) client bound to an executor and optional base URL (e.g. \c "http://127.0.0.1:8080").
class HttpClient {
 public:
  /// Plain HTTP. \p base_url must start with \c http:// (default port 80 if omitted).
  explicit HttpClient(boost::asio::any_io_executor ex, std::string base_url = "http://127.0.0.1:80");

#if defined(USING_TLS)
  /// HTTPS-capable client; \p base_url may use \c https:// when \p ssl_ctx is provided.
  HttpClient(boost::asio::any_io_executor ex, std::string base_url, boost::asio::ssl::context &ssl_ctx);
#endif

  /// Set client options (timeouts, etc.)
  HttpClient &set_options(const ClientOptions &opts);

  /// Get current client options
  const ClientOptions &get_options() const { return opts_; }

  [[nodiscard]] RequestBuilder get(std::string_view target_or_url);
  [[nodiscard]] RequestBuilder post(std::string_view target_or_url);
  [[nodiscard]] RequestBuilder put(std::string_view target_or_url);
  [[nodiscard]] RequestBuilder delete_(std::string_view target_or_url);

  boost::asio::any_io_executor get_executor() const { return ex_; }

 private:
  friend class RequestBuilder;

  boost::asio::any_io_executor ex_;
  detail::ClientBaseConfig cfg_;
  ClientOptions opts_;
#if defined(USING_TLS)
  boost::asio::ssl::context *ssl_ctx_{nullptr};
#endif
};

/// Accumulates headers/body and completes with \c then / \c catch_error or \c co_await as_awaitable().
class RequestBuilder {
 public:
  RequestBuilder(const HttpClient &owner, http::verb method, std::string target_or_url);

  RequestBuilder &header(std::string name, std::string value);
  RequestBuilder &body(std::string body);
  RequestBuilder &body_json(const nlohmann::json &j);

  /// Set timeout options for this request
  RequestBuilder &timeout(const RequestTimeoutOptions &opts);

  /// Set Connection timeout for this request
  RequestBuilder &connection_timeout(std::chrono::milliseconds timeout);

  /// Set request timeout for this request
  RequestBuilder &request_timeout(std::chrono::milliseconds timeout);

  /// Register transport/parser error handler (optional). Call before \ref then if used.
  RequestBuilder &catch_error(std::function<void(const boost::system::error_code &)> fn);

  /// Starts the request and invokes \p fn with the response on success.
  RequestBuilder &then(std::function<void(const http::response<http::string_body> &)> fn);

  /// Coroutine entry: do not use with \ref then on the same builder.
  /// Only available when built with C++20 (FOXHTTP_HAS_COROUTINES = 1).
#if FOXHTTP_HAS_COROUTINES
  [[nodiscard]] boost::asio::awaitable<http::response<http::string_body>> as_awaitable();
#endif

 private:
  void ensure_not_consumed();

  boost::asio::any_io_executor ex_;
  std::shared_ptr<detail::RequestSpec> spec_;
  RequestTimeoutOptions timeout_opts_;
  bool consumed_{false};
};

}  // namespace foxhttp::client
