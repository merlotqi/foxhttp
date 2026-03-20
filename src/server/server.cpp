/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <boost/asio/spawn.hpp>
#include <boost/system/detail/error_code.hpp>
#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/io_context_pool.hpp>
#include <foxhttp/server/server.hpp>
#include <foxhttp/server/session.hpp>


namespace foxhttp {

server::server(io_context_pool &io_pool, unsigned short port)
    : io_pool_(io_pool),
      listen_io_(&io_pool_.get_io_context()),
      acceptor_(*listen_io_, {tcp::v4(), port}),
      global_chain_(std::make_shared<middleware_chain>(*listen_io_)) {
  _do_accept();
}

void server::use(std::shared_ptr<middleware> mw) { global_chain_->use(std::move(mw)); }

std::shared_ptr<middleware_chain> server::global_chain() const { return global_chain_; }

void server::_do_accept() {
  auto &io_context = io_pool_.get_io_context();
  acceptor_.async_accept(io_context, [this](beast::error_code ec, tcp::socket socket) {
    if (!ec) {
      std::make_shared<session>(std::move(socket), global_chain_)->start();
    }
    _do_accept();
  });
}

}  // namespace foxhttp
