#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/client/client_options.hpp>
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

/// Fluent HTTP/HTTPS client (Promise-style \ref request_builder::then / \ref request_builder::catch_error,
/// or \ref request_builder::as_awaitable for coroutines). One-shot per builder: do not mix \c then() and \c
/// as_awaitable().
class request_builder;

/// HTTP(S) client bound to an executor and optional base URL (e.g. \c "http://127.0.0.1:8080").
class http_client {
 public:
  /// Plain HTTP. \p base_url must start with \c http:// (default port 80 if omitted).
  explicit http_client(boost::asio::any_io_executor ex, std::string base_url = "http://127.0.0.1:80");

#if defined(USING_TLS)
  /// HTTPS-capable client; \p base_url may use \c https:// when \p ssl_ctx is provided.
  http_client(boost::asio::any_io_executor ex, std::string base_url, boost::asio::ssl::context &ssl_ctx);
#endif

  /// Set client options (timeouts, etc.)
  http_client &set_options(const client_options &opts);

  /// Get current client options
  const client_options &get_options() const { return opts_; }

  [[nodiscard]] request_builder get(std::string_view target_or_url);
  [[nodiscard]] request_builder post(std::string_view target_or_url);
  [[nodiscard]] request_builder put(std::string_view target_or_url);
  [[nodiscard]] request_builder delete_(std::string_view target_or_url);

  boost::asio::any_io_executor get_executor() const { return ex_; }

 private:
  friend class request_builder;

  boost::asio::any_io_executor ex_;
  detail::client_base_config cfg_;
  client_options opts_;
#if defined(USING_TLS)
  boost::asio::ssl::context *ssl_ctx_{nullptr};
#endif
};

/// Accumulates headers/body and completes with \c then / \c catch_error or \c co_await as_awaitable().
class request_builder {
 public:
  request_builder(const http_client &owner, http::verb method, std::string target_or_url);

  request_builder &header(std::string name, std::string value);
  request_builder &body(std::string body);
  request_builder &body_json(const nlohmann::json &j);

  /// Set timeout options for this request
  request_builder &timeout(const request_timeout_options &opts);

  /// Set connection timeout for this request
  request_builder &connection_timeout(std::chrono::milliseconds timeout);

  /// Set request timeout for this request
  request_builder &request_timeout(std::chrono::milliseconds timeout);

  /// Register transport/parser error handler (optional). Call before \ref then if used.
  request_builder &catch_error(std::function<void(const boost::system::error_code &)> fn);

  /// Starts the request and invokes \p fn with the response on success.
  request_builder &then(std::function<void(const http::response<http::string_body> &)> fn);

  /// Coroutine entry: do not use with \ref then on the same builder.
  [[nodiscard]] boost::asio::awaitable<http::response<http::string_body>> as_awaitable();

 private:
  void ensure_not_consumed();

  boost::asio::any_io_executor ex_;
  std::shared_ptr<detail::request_spec> spec_;
  request_timeout_options timeout_opts_;
  bool consumed_{false};
};

}  // namespace foxhttp::client
