#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/system/error_code.hpp>
#include <foxhttp/client/http_client.hpp>
#include <foxhttp/constants.hpp>
#include <spdlog/spdlog.h>

#if defined(USING_TLS)
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#endif

#include <cctype>
#include <chrono>
#include <stdexcept>
#include <string_view>

namespace foxhttp::client {

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

namespace {

constexpr int k_default_http_port = 80;
constexpr int k_default_https_port = 443;

bool ci_eq(char a, char b) {
  return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
}

bool starts_with_ci(std::string_view s, std::string_view p) {
  if (s.size() < p.size()) return false;
  for (std::size_t i = 0; i < p.size(); ++i) {
    if (!ci_eq(s[i], p[i])) return false;
  }
  return true;
}

/// \p url is full http(s) URL; fills \p out_base and \p out_target (path + query).
bool parse_absolute_url(std::string url, detail::client_base_config &out_base, std::string &out_target) {
  if (starts_with_ci(url, "https://")) {
#if !defined(USING_TLS)
    return false;
#else
    out_base.https = true;
    url.erase(0, 8);
#endif
  } else if (starts_with_ci(url, "http://")) {
    out_base.https = false;
    url.erase(0, 7);
  } else {
    return false;
  }

  const auto path_pos = url.find('/');
  const auto authority = path_pos == std::string::npos ? url : url.substr(0, path_pos);
  out_target = path_pos == std::string::npos ? "/" : url.substr(path_pos);

  const auto colon = authority.rfind(':');
  if (colon != std::string::npos) {
    out_base.host = authority.substr(0, colon);
    out_base.port = authority.substr(colon + 1);
  } else {
    out_base.host = authority;
    out_base.port = out_base.https ? std::to_string(k_default_https_port) : std::to_string(k_default_http_port);
  }

  if (out_base.host.empty()) return false;
  if (out_base.https) {
    out_base.host_field = authority;
  } else {
    out_base.host_field = (out_base.port == std::to_string(k_default_http_port)) ? out_base.host : authority;
  }
  return true;
}

bool parse_base_only(std::string url, detail::client_base_config &out_base) {
  std::string dummy;
  if (!parse_absolute_url(std::move(url), out_base, dummy)) return false;
  return true;
}

std::string normalize_relative_target(std::string_view t) {
  if (t.empty()) return "/";
  if (t.front() == '/') return std::string(t);
  return std::string("/") + std::string(t);
}

asio::awaitable<void> transfer_http(std::shared_ptr<detail::request_spec> spec, http::response<http::string_body> &res,
                                    beast::error_code &ec) {
  const auto &b = spec->base;
  tcp::resolver resolver(co_await asio::this_coro::executor);
  const auto results = co_await resolver.async_resolve(b.host, b.port, asio::redirect_error(asio::use_awaitable, ec));
  if (ec) co_return;

  beast::tcp_stream stream(co_await asio::this_coro::executor);
  stream.expires_after(constants::kDefaultIoTimeout);
  co_await stream.async_connect(results, asio::redirect_error(asio::use_awaitable, ec));
  if (ec) co_return;

  http::request<http::string_body> req{spec->method, spec->target_path, 11};
  req.set(http::field::host, b.host_field);
  req.set(http::field::user_agent, std::string("FoxHttp-client/") + BOOST_BEAST_VERSION_STRING);
  for (const auto &kv : spec->headers) req.insert(kv.first, kv.second);
  req.body() = spec->body;
  req.prepare_payload();

  co_await http::async_write(stream, req, asio::redirect_error(asio::use_awaitable, ec));
  if (ec) co_return;

  beast::flat_buffer buffer;
  co_await http::async_read(stream, buffer, res, asio::redirect_error(asio::use_awaitable, ec));
  if (ec) co_return;

  try {
    stream.socket().shutdown(tcp::socket::shutdown_both);
  } catch (const boost::system::system_error &e) {
    spdlog::warn("Failed to shutdown socket after HTTP transfer: {}", e.what());
  }
}

#if defined(USING_TLS)
asio::awaitable<void> transfer_https(std::shared_ptr<detail::request_spec> spec, http::response<http::string_body> &res,
                                     beast::error_code &ec) {
  if (!spec->ssl_ctx) {
    ec = asio::error::operation_not_supported;
    co_return;
  }
  const auto &b = spec->base;
  tcp::resolver resolver(co_await asio::this_coro::executor);
  const auto results = co_await resolver.async_resolve(b.host, b.port, asio::redirect_error(asio::use_awaitable, ec));
  if (ec) co_return;

  beast::tcp_stream plain(co_await asio::this_coro::executor);
  plain.expires_after(constants::kDefaultIoTimeout);
  co_await plain.async_connect(results, asio::redirect_error(asio::use_awaitable, ec));
  if (ec) co_return;

  beast::ssl_stream<beast::tcp_stream> stream(std::move(plain), *spec->ssl_ctx);
  stream.next_layer().expires_after(constants::kDefaultHandshakeTimeout);
  co_await stream.async_handshake(asio::ssl::stream_base::client, asio::redirect_error(asio::use_awaitable, ec));
  if (ec) co_return;

  http::request<http::string_body> req{spec->method, spec->target_path, 11};
  req.set(http::field::host, b.host_field);
  req.set(http::field::user_agent, std::string("FoxHttp-client/") + BOOST_BEAST_VERSION_STRING);
  for (const auto &kv : spec->headers) req.insert(kv.first, kv.second);
  req.body() = spec->body;
  req.prepare_payload();

  co_await http::async_write(stream, req, asio::redirect_error(asio::use_awaitable, ec));
  if (ec) co_return;

  beast::flat_buffer buffer;
  co_await http::async_read(stream, buffer, res, asio::redirect_error(asio::use_awaitable, ec));
  if (ec) co_return;

  try {
    stream.next_layer().socket().shutdown(tcp::socket::shutdown_both);
  } catch (const boost::system::system_error &e) {
    spdlog::warn("Failed to shutdown socket after HTTPS transfer: {}", e.what());
  }
}
#endif

asio::awaitable<void> transfer(std::shared_ptr<detail::request_spec> spec, http::response<http::string_body> &res,
                               beast::error_code &ec) {
#if defined(USING_TLS)
  if (spec->base.https) {
    co_await transfer_https(spec, res, ec);
    co_return;
  }
#endif
  co_await transfer_http(spec, res, ec);
}

asio::awaitable<void> run_callbacks(std::shared_ptr<detail::request_spec> spec) {
  http::response<http::string_body> res;
  beast::error_code ec;
  co_await transfer(spec, res, ec);

  auto ex = co_await asio::this_coro::executor;
  if (ec) {
    if (spec->on_error) {
      asio::post(ex, [spec, ec] { spec->on_error(ec); });
    }
  } else {
    if (spec->on_success) {
      asio::post(ex, [spec, res] { spec->on_success(res); });
    }
  }
}

asio::awaitable<http::response<http::string_body>> transfer_await(std::shared_ptr<detail::request_spec> spec) {
  http::response<http::string_body> res;
  beast::error_code ec;
  co_await transfer(spec, res, ec);
  if (ec) throw boost::system::system_error(ec);
  co_return res;
}

}  // namespace

// ---------- http_client ----------

http_client::http_client(asio::any_io_executor ex, std::string base_url) : ex_(std::move(ex)) {
  if (!parse_base_only(std::move(base_url), cfg_)) {
    throw std::invalid_argument("foxhttp::client::http_client: invalid base URL (expected http://host[:port])");
  }
}

#if defined(USING_TLS)
http_client::http_client(asio::any_io_executor ex, std::string base_url, asio::ssl::context &ssl_ctx)
    : ex_(std::move(ex)), ssl_ctx_(&ssl_ctx) {
  if (!parse_base_only(std::move(base_url), cfg_)) {
    throw std::invalid_argument("foxhttp::client::http_client: invalid base URL");
  }
}
#endif

http_client &http_client::set_options(const client_options &opts) {
  opts_ = opts;
  return *this;
}

request_builder http_client::get(std::string_view target_or_url) {
  return request_builder(*this, http::verb::get, std::string(target_or_url));
}

request_builder http_client::post(std::string_view target_or_url) {
  return request_builder(*this, http::verb::post, std::string(target_or_url));
}

request_builder http_client::put(std::string_view target_or_url) {
  return request_builder(*this, http::verb::put, std::string(target_or_url));
}

request_builder http_client::delete_(std::string_view target_or_url) {
  return request_builder(*this, http::verb::delete_, std::string(target_or_url));
}

// ---------- request_builder ----------

request_builder::request_builder(const http_client &owner, http::verb method, std::string target_or_url)
    : ex_(owner.get_executor()), spec_(std::make_shared<detail::request_spec>()) {
  spec_->method = method;
#if defined(USING_TLS)
  spec_->ssl_ctx = owner.ssl_ctx_;
#endif
  const std::string t = std::move(target_or_url);
  if (starts_with_ci(t, "http://") || starts_with_ci(t, "https://")) {
    if (!parse_absolute_url(t, spec_->base, spec_->target_path)) {
      throw std::invalid_argument("foxhttp::client: invalid absolute URL");
    }
#if !defined(USING_TLS)
    if (spec_->base.https) {
      throw std::invalid_argument("foxhttp::client: HTTPS requires building FoxHttp with TLS enabled");
    }
#endif
  } else {
    spec_->base = owner.cfg_;
    spec_->target_path = normalize_relative_target(t);
  }
}

request_builder &request_builder::header(std::string name, std::string value) {
  ensure_not_consumed();
  spec_->headers.emplace_back(std::move(name), std::move(value));
  return *this;
}

request_builder &request_builder::body(std::string body) {
  ensure_not_consumed();
  spec_->body = std::move(body);
  return *this;
}

request_builder &request_builder::body_json(const nlohmann::json &j) {
  ensure_not_consumed();
  spec_->headers.emplace_back("Content-Type", "application/json");
  spec_->body = j.dump();
  return *this;
}

request_builder &request_builder::timeout(const request_timeout_options &opts) {
  ensure_not_consumed();
  timeout_opts_ = opts;
  return *this;
}

request_builder &request_builder::connection_timeout(std::chrono::milliseconds timeout) {
  ensure_not_consumed();
  timeout_opts_.connection_timeout = timeout;
  return *this;
}

request_builder &request_builder::request_timeout(std::chrono::milliseconds timeout) {
  ensure_not_consumed();
  timeout_opts_.request_timeout = timeout;
  return *this;
}

request_builder &request_builder::catch_error(std::function<void(const boost::system::error_code &)> fn) {
  ensure_not_consumed();
  spec_->on_error = std::move(fn);
  return *this;
}

request_builder &request_builder::then(std::function<void(const http::response<http::string_body> &)> fn) {
  ensure_not_consumed();
  spec_->on_success = std::move(fn);
  consumed_ = true;
  asio::co_spawn(ex_, run_callbacks(spec_), asio::detached);
  return *this;
}

asio::awaitable<http::response<http::string_body>> request_builder::as_awaitable() {
  ensure_not_consumed();
  consumed_ = true;
  return transfer_await(spec_);
}

void request_builder::ensure_not_consumed() {
  if (consumed_) {
    throw std::logic_error("foxhttp::client::request_builder: request already started (do not reuse)");
  }
}

}  // namespace foxhttp::client
