#include <boost/asio/error.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/request_context.hpp>
#include <foxhttp/server/ssl_session.hpp>
#if USING_TLS
#include <foxhttp/server/wss_session.hpp>
#endif
#include <spdlog/spdlog.h>

namespace beast = boost::beast;
namespace http = beast::http;

namespace foxhttp {

ssl_session::ssl_session(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream,
                         std::shared_ptr<middleware_chain> global_chain)
    : session_base(stream.get_executor(), global_chain),
      stream_(std::move(stream)),
      global_chain_(std::move(global_chain)) {}

void ssl_session::start() {
  arm_idle_timer();
  handshake();
}

void ssl_session::handshake() {
  auto self = shared_from_this();
  stream_.async_handshake(boost::asio::ssl::stream_base::server, [self](boost::system::error_code ec) {
    if (ec) {
      if (ec != boost::asio::error::operation_aborted) {
        spdlog::warn("foxhttp ssl_session handshake error: {}", ec.message());
      }
      return;
    }
    self->on_activity();
    self->arm_header_timer();
    self->read_request();
  });
}

void ssl_session::read_request() {
  auto self = shared_from_this();
  http::async_read(stream_, buffer_, req_, [self](beast::error_code ec, std::size_t bytes_transferred) {
    self->handle_read(ec, bytes_transferred);
  });
}

void ssl_session::handle_read(beast::error_code ec, std::size_t) {
  if (ec) {
    if (ec != http::error::end_of_stream && ec != boost::asio::error::operation_aborted) {
      spdlog::warn("foxhttp ssl_session read error: {}", ec.message());
    }
    return;
  }
  cancel_header_timer();
  on_activity();
  process_request();
}

void ssl_session::process_request() {
  auto self = shared_from_this();
  request_context ctx(req_);

#ifdef USING_TLS

  // WebSocket upgrade detection (wss)
  if (boost::beast::websocket::is_upgrade(req_)) {
    using tls_stream = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
    boost::beast::websocket::stream<tls_stream> ws(std::move(stream_));
    auto wss_sess = std::make_shared<wss_session>(std::move(ws));
    wss_sess->start_accept(req_);
    return;
  }
#endif
  global_chain_->execute_async(ctx, res_, [self](middleware_result, const std::string &) { self->write_response(); });
}

void ssl_session::write_response() {
  auto self = shared_from_this();
  res_.prepare_payload();
  http::async_write(stream_, res_, [self](beast::error_code ec, std::size_t) {
    if (ec) {
      if (ec != boost::asio::error::operation_aborted) {
        spdlog::warn("foxhttp ssl_session write error: {}", ec.message());
      }
      beast::error_code sec;
      self->stream_.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, sec);
      return;
    }

    ++self->requests_served_;
    const auto &lim = self->limits();
    const bool keep_open =
        lim.enable_keep_alive && self->res_.keep_alive() &&
        (lim.max_requests_per_connection == 0 || self->requests_served_ < lim.max_requests_per_connection);
    if (!keep_open) {
      beast::error_code sec;
      self->stream_.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, sec);
      return;
    }

    const unsigned req_ver = self->req_.version();
    self->req_ = {};
    self->res_ = http::response<http::string_body>{http::status::ok, req_ver};
    self->on_activity();
    self->arm_header_timer();
    self->read_request();
  });
}

void ssl_session::on_timeout_idle() {
  beast::error_code ec;
  stream_.next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
}

void ssl_session::on_timeout_header()

{
  // respond 408 Request Timeout
  res_.result(http::status::request_timeout);
  res_.set(http::field::content_type, "text/plain");
  res_.body() = "Header read timeout";
  write_response();
}

void ssl_session::on_timeout_body() {
  res_.result(http::status::request_timeout);
  res_.set(http::field::content_type, "text/plain");
  res_.body() = "Body read timeout";
  write_response();
}

}  // namespace foxhttp
