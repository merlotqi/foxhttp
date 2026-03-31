#pragma once

#include <atomic>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>

using tcp = boost::asio::ip::tcp;

namespace foxhttp {

class middleware;
class io_context_pool;
class middleware_chain;

class server {
 public:
  server(io_context_pool &io_pool, unsigned short port);

  void use(std::shared_ptr<middleware> mw);
  std::shared_ptr<middleware_chain> global_chain() const;

  void stop();

 private:
  void do_accept();
  boost::asio::awaitable<void> accept_loop();

  io_context_pool &io_pool_;
  boost::asio::io_context *listen_io_;
  tcp::acceptor acceptor_;
  std::shared_ptr<middleware_chain> global_chain_;
  std::atomic<bool> stopping_{false};
};

}  // namespace foxhttp
