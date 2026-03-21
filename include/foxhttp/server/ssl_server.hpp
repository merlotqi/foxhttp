#pragma once

#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <memory>

namespace foxhttp {

class io_context_pool;
class middleware_chain;

class ssl_server {
 public:
  ssl_server(io_context_pool &io_pool, unsigned short port, boost::asio::ssl::context &ssl_ctx);

  void use(std::shared_ptr<middleware_chain> chain);
  std::shared_ptr<middleware_chain> global_chain() const;

 private:
  void do_accept();
  boost::asio::awaitable<void> accept_loop();

 private:
  io_context_pool &io_pool_;
  boost::asio::io_context *listen_io_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ssl::context &ssl_ctx_;
  std::shared_ptr<middleware_chain> global_chain_;
};

}  // namespace foxhttp
