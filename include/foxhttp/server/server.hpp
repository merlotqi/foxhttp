/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

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

 private:
  void _do_accept();

  io_context_pool &io_pool_;
  boost::asio::io_context *listen_io_;
  tcp::acceptor acceptor_;
  std::shared_ptr<middleware_chain> global_chain_;
};

}  // namespace foxhttp
