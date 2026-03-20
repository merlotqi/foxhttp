/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * middleware utilities and helper classes
 */

#pragma once

#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/server/session_base.hpp>
#include <memory>

namespace foxhttp {

class middleware_chain;

class ssl_session : public session_base, public std::enable_shared_from_this<ssl_session> {
 public:
  explicit ssl_session(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream,
                       std::shared_ptr<middleware_chain> global_chain);
  void start();

 private:
  void handshake();
  void read_request();
  void handle_read(boost::beast::error_code ec, std::size_t bytes_transferred);
  void process_request();
  void write_response();

  void on_timeout_idle() override;
  void on_timeout_header() override;
  void on_timeout_body() override;

 private:
  boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream_;
  boost::beast::flat_buffer buffer_;
  boost::beast::http::request<boost::beast::http::string_body> req_;
  boost::beast::http::response<boost::beast::http::string_body> res_;
  std::shared_ptr<middleware_chain> global_chain_;
  std::size_t requests_served_{0};
};

}  // namespace foxhttp
