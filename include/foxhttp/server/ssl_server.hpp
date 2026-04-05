#pragma once

#include <atomic>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <foxhttp/config.hpp>
#include <memory>

#if FOXHTTP_HAS_COROUTINES
#include <boost/asio/awaitable.hpp>
#endif

namespace foxhttp::middleware {
class MiddlewareChain;
}

namespace foxhttp::server {

class IoContextPool;

using MiddlewareChain = middleware::MiddlewareChain;

class SslServer {
 public:
  SslServer(IoContextPool &io_pool, unsigned short port, boost::asio::ssl::context &ssl_ctx);

  void use(std::shared_ptr<MiddlewareChain> chain);
  std::shared_ptr<MiddlewareChain> global_chain() const;

  void stop();

 private:
  void do_accept();
#if FOXHTTP_HAS_COROUTINES
  boost::asio::awaitable<void> accept_loop();
#else
  void accept_loop();
#endif

 private:
  IoContextPool &io_pool_;
  boost::asio::io_context *listen_io_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ssl::context &ssl_ctx_;
  std::shared_ptr<MiddlewareChain> global_chain_;
  std::atomic<bool> stopping_{false};
};

}  // namespace foxhttp::server
