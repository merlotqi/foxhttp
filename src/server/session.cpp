#include <spdlog/spdlog.h>

#include <boost/asio/error.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/websocket.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/request_context.hpp>
#include <foxhttp/server/session.hpp>
#include <foxhttp/server/ws_session.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace foxhttp {

session::session(tcp::socket socket, std::shared_ptr<middleware_chain> global_chain)
    : session_base(socket.get_executor(), global_chain),
      socket_(std::move(socket)),
      global_chain_(std::move(global_chain)) {}

void session::start() {
  arm_idle_timer();
  arm_header_timer();
  read_request();
}

void session::read_request() {
  auto self = shared_from_this();
  http::async_read(socket_, buffer_, req_, [self](beast::error_code ec, std::size_t bytes_transferred) {
    self->handle_read(ec, bytes_transferred);
  });
}

void session::handle_read(beast::error_code ec, size_t) {
  if (ec) {
    if (ec != http::error::end_of_stream && ec != boost::asio::error::operation_aborted) {
      spdlog::warn("foxhttp session read error: {}", ec.message());
    }
    return;
  }
  cancel_header_timer();
  on_activity();
  process_request();
}

void session::process_request() {
  auto self = shared_from_this();
  request_context ctx(req_);
  // WebSocket upgrade detection
  bool is_ws = boost::beast::websocket::is_upgrade(req_);
  if (is_ws) {
    // Create WebSocket stream using beast::tcp_stream to satisfy teardown
    boost::beast::tcp_stream ts(std::move(socket_));
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws(std::move(ts));
    auto ws_sess = std::make_shared<ws_session>(std::move(ws));
    // ws_session handles its own lifecycle
    ws_sess->start_accept(req_);
    return;
  }
  global_chain_->execute_async(ctx, res_, [self](middleware_result, const std::string &) { self->write_response(); });
}

void session::write_response() {
  auto self = shared_from_this();
  res_.prepare_payload();
  http::async_write(socket_, res_, [self](beast::error_code ec, std::size_t) {
    if (ec) {
      if (ec != boost::asio::error::operation_aborted) {
        spdlog::warn("foxhttp session write error: {}", ec.message());
      }
      beast::error_code sec;
      self->socket_.shutdown(tcp::socket::shutdown_both, sec);
      return;
    }

    ++self->requests_served_;
    const auto &lim = self->limits();
    const bool keep_open =
        lim.enable_keep_alive && self->res_.keep_alive() &&
        (lim.max_requests_per_connection == 0 || self->requests_served_ < lim.max_requests_per_connection);
    if (!keep_open) {
      beast::error_code sec;
      self->socket_.shutdown(tcp::socket::shutdown_both, sec);
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

void session::on_timeout_idle() {
  beast::error_code ec;
  socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
}

void session::on_timeout_header() {
  res_.result(http::status::request_timeout);
  res_.set(http::field::content_type, "text/plain");
  res_.body() = "Header read timeout";
  write_response();
}

void session::on_timeout_body() {
  res_.result(http::status::request_timeout);
  res_.set(http::field::content_type, "text/plain");
  res_.body() = "Body read timeout";
  write_response();
}

}  // namespace foxhttp
