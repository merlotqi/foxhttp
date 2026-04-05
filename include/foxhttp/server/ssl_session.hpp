#pragma once

#include <atomic>
#include <boost/asio/awaitable.hpp>
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
  boost::asio::awaitable<void> run();
  void arm_handshake_timer();
  void cancel_handshake_timer();

  enum class read_abort_reason {
    none,
    header_timeout,
    body_timeout,
    handshake_timeout
  };
  std::atomic<read_abort_reason> read_abort_{read_abort_reason::none};

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
  boost::asio::steady_timer handshake_timer_;
};

}  // namespace foxhttp
