#include <spdlog/spdlog.h>

#include <foxhttp/config.hpp>
#include <foxhttp/server/wss_session.hpp>

#if FOXHTTP_HAS_COROUTINES
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#endif
#include <boost/beast/websocket/ssl.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
namespace asio = boost::asio;

namespace foxhttp::server {

WssSession::WssSession(websocket_t ws, const WebSocketLimits &wsl, const SessionLimits &sl)
    : SessionBase(ws.get_executor(), nullptr, sl), ws_(std::move(ws)), wsl_(wsl) {
  ws_.read_message_max(wsl_.max_message_bytes);
}

void WssSession::start_accept(http::request<http::string_body> req) {
  arm_idle_timer();
  ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
#if FOXHTTP_HAS_COROUTINES
  asio::co_spawn(
      ws_.get_executor(),
      [self = shared_from_this(), req = std::move(req)]() mutable -> asio::awaitable<void> {
        boost::system::error_code ec;
        co_await self->ws_.async_accept(req, asio::redirect_error(asio::use_awaitable, ec));
        if (ec) co_return;
        co_await self->echo_loop();
      },
      asio::detached);
#else
  auto self = shared_from_this();
  ws_.async_accept(std::move(req), [self](beast::error_code ec) {
    if (ec) return;
    self->echo_loop();
  });
#endif
}

#if FOXHTTP_HAS_COROUTINES
asio::awaitable<void> WssSession::echo_loop() {
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
#else
void WssSession::echo_loop() {
  auto self = shared_from_this();
  ws_.async_read(buffer_, [this, self](beast::error_code ec, std::size_t /*bytes*/) {
    if (ec) return;
    ws_.text(ws_.got_text());
    ws_.async_write(buffer_.data(), [this, self](beast::error_code ec, std::size_t /*bytes*/) {
      if (ec) return;
      buffer_.consume(buffer_.size());
      // Continue echoing
      echo_loop();
    });
  });
}
#endif

void WssSession::on_timeout_idle() {
  try {
    ws_.next_layer().next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
  } catch (const boost::system::system_error &e) {
    spdlog::warn("Failed to shutdown socket on idle timeout: {}", e.what());
  }
}

}  // namespace foxhttp::server
