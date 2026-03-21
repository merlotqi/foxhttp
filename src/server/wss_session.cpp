#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <foxhttp/server/wss_session.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
namespace asio = boost::asio;

namespace foxhttp {

wss_session::wss_session(websocket_t ws, const websocket_limits &wsl, const session_limits &sl)
    : session_base(ws.get_executor(), nullptr, sl), ws_(std::move(ws)), wsl_(wsl) {
  ws_.read_message_max(wsl_.max_message_bytes);
}

void wss_session::start_accept(http::request<http::string_body> req) {
  arm_idle_timer();
  ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
  asio::co_spawn(
      ws_.get_executor(),
      [self = shared_from_this(), req = std::move(req)]() mutable -> asio::awaitable<void> {
        boost::system::error_code ec;
        co_await self->ws_.async_accept(req, asio::redirect_error(asio::use_awaitable, ec));
        if (ec) co_return;
        co_await self->echo_loop();
      },
      asio::detached);
}

asio::awaitable<void> wss_session::echo_loop() {
  while (true) {
    boost::system::error_code ec;
    co_await ws_.async_read(buffer_, asio::redirect_error(asio::use_awaitable, ec));
    if (ec) co_return;
    ws_.text(ws_.got_text());
    co_await ws_.async_write(buffer_.data(), asio::redirect_error(asio::use_awaitable, ec));
    if (ec) co_return;
    buffer_.consume(buffer_.size());
  }
}

void wss_session::on_timeout_idle() {
  beast::error_code ec;
  ws_.next_layer().next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
}

}  // namespace foxhttp
