#pragma once

#include <atomic>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/config.hpp>
#include <foxhttp/server/session_base.hpp>
#include <memory>

namespace foxhttp::middleware {
class MiddlewareChain;
}

#if FOXHTTP_HAS_COROUTINES
#include <boost/asio/awaitable.hpp>
#endif

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

namespace foxhttp::server {

using MiddlewareChain = middleware::MiddlewareChain;

class Session : public SessionBase, public std::enable_shared_from_this<Session> {
 public:
  explicit Session(tcp::socket socket, std::shared_ptr<MiddlewareChain> global_chain);
  void start();

 private:
#if FOXHTTP_HAS_COROUTINES
  boost::asio::awaitable<void> run();
#else
  void run();
  void do_read_request();
#endif

  enum class ReadAbortReason {
    None,
    HeaderTimeout,
    BodyTimeout
  };
  std::atomic<ReadAbortReason> read_abort_{ReadAbortReason::None};

  void on_timeout_idle() override;
  void on_timeout_header() override;
  void on_timeout_body() override;

 private:
  tcp::socket socket_;
  boost::beast::flat_buffer buffer_;
  http::request<http::string_body> req_;
  http::response<http::string_body> res_;
  std::shared_ptr<MiddlewareChain> global_chain_;
  std::size_t requests_served_{0};
};

}  // namespace foxhttp::server
