/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * middleware utilities and helper classes
 */

#pragma once

#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <foxhttp/config/configs.hpp>
#include <foxhttp/server/session_base.hpp>

namespace foxhttp {

class wss_session : public session_base, public std::enable_shared_from_this<wss_session> {
 public:
  using stream_t = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
  using websocket_t = boost::beast::websocket::stream<stream_t>;

  wss_session(websocket_t ws, const websocket_limits &wsl = {}, const session_limits &sl = {});

  void start_accept(boost::beast::http::request<boost::beast::http::string_body> req);

 private:
  void _on_accept(boost::system::error_code ec);
  void _do_read();
  void _on_read(boost::system::error_code ec, std::size_t bytes_transferred);
  void on_timeout_idle() override;

 private:
  websocket_t ws_;
  websocket_limits wsl_;
  boost::beast::flat_buffer buffer_;
};

}  // namespace foxhttp
