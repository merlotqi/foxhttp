/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * middleware utilities and helper classes
 */

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
  _read_request();
}

void session::_read_request() {
  auto self = shared_from_this();
  http::async_read(socket_, buffer_, req_, [self](beast::error_code ec, std::size_t bytes_transferred) {
    self->_handle_read(ec, bytes_transferred);
  });
}

void session::_handle_read(beast::error_code ec, size_t) {
  if (ec) {
    return;
  }
  cancel_header_timer();
  on_activity();
  _process_request();
}

void session::_process_request() {
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
  global_chain_->execute_async(ctx, res_, [self](middleware_result, const std::string &) { self->_write_response(); });
}

void session::_write_response() {
  auto self = shared_from_this();
  res_.prepare_payload();
  http::async_write(socket_, res_, [self](beast::error_code, std::size_t) {
    beast::error_code ec;
    self->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
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
  _write_response();
}

void session::on_timeout_body() {
  res_.result(http::status::request_timeout);
  res_.set(http::field::content_type, "text/plain");
  res_.body() = "Body read timeout";
  _write_response();
}

}  // namespace foxhttp
