#pragma once

#include <atomic>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/config.hpp>
#include <foxhttp/server/session_base.hpp>
#include <memory>

#if FOXHTTP_HAS_COROUTINES
#include <boost/asio/awaitable.hpp>
#endif

namespace foxhttp::server {

using MiddlewareChain = middleware::MiddlewareChain;

class SslSession : public SessionBase, public std::enable_shared_from_this<SslSession> {
 public:
  explicit SslSession(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream,
                       std::shared_ptr<MiddlewareChain> global_chain);
  void start();

 private:
#if FOXHTTP_HAS_COROUTINES
  boost::asio::awaitable<void> run();
#else
  void run();
  void do_read_request();
#endif
  void arm_handshake_timer();
  void cancel_handshake_timer();

  enum class ReadAbortReason {
    None,
    HeaderTimeout,
    BodyTimeout,
    HandshakeTimeout
  };
  std::atomic<ReadAbortReason> read_abort_{ReadAbortReason::None};

  void on_timeout_idle() override;
  void on_timeout_header() override;
  void on_timeout_body() override;

 private:
  boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream_;
  boost::beast::flat_buffer buffer_;
  boost::beast::http::request<boost::beast::http::string_body> req_;
  boost::beast::http::response<boost::beast::http::string_body> res_;
  std::shared_ptr<MiddlewareChain> global_chain_;
  std::size_t requests_served_{0};
  boost::asio::steady_timer handshake_timer_;
};

}  // namespace foxhttp::server
