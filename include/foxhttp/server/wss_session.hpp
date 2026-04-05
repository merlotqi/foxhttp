#pragma once

#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <foxhttp/config.hpp>
#include <foxhttp/config/configs.hpp>
#include <foxhttp/server/session_base.hpp>

#if FOXHTTP_HAS_COROUTINES
#include <boost/asio/awaitable.hpp>
#endif

namespace foxhttp::server {

using WebSocketLimits = config::WebSocketLimits;

class WssSession : public SessionBase, public std::enable_shared_from_this<WssSession> {
 public:
  using stream_t = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
  using websocket_t = boost::beast::websocket::stream<stream_t>;

  WssSession(websocket_t ws, const WebSocketLimits &wsl = {}, const SessionLimits &sl = {});

  void start_accept(boost::beast::http::request<boost::beast::http::string_body> req);

 private:
#if FOXHTTP_HAS_COROUTINES
  boost::asio::awaitable<void> echo_loop();
#else
  void echo_loop();
#endif

  void on_timeout_idle() override;

 private:
  websocket_t ws_;
  WebSocketLimits wsl_;
  boost::beast::flat_buffer buffer_;
};

}  // namespace foxhttp::server
