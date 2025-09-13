/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <foxhttp/core/io_context_pool.hpp>
#include <foxhttp/core/server.hpp>
#include <foxhttp/core/session.hpp>
#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>

namespace foxhttp {

Server::Server(IOContextPool &io_pool, unsigned short port)
    : io_pool_(io_pool), acceptor_(io_pool_.get_io_context(), {tcp::v4(), port}),
      global_chain_(std::make_shared<MiddlewareChain>())
{
    do_accept();
}

void Server::use(std::shared_ptr<Middleware> mw)
{
    global_chain_->use(std::move(mw));
}

std::shared_ptr<MiddlewareChain> Server::global_chain() const
{
    return global_chain_;
}

void Server::do_accept()
{
    auto &io_context = io_pool_.get_io_context();
    acceptor_.async_accept(io_context, [this](beast::error_code ec, tcp::socket socket) {
        if (!ec)
        {
            std::make_shared<Session>(std::move(socket), global_chain_)->start();
        }
        do_accept();
    });
}

}// namespace foxhttp
