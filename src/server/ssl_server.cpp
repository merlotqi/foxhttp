/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * middleware utilities and helper classes
 */

#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/io_context_pool.hpp>
#include <foxhttp/server/ssl_server.hpp>
#include <foxhttp/server/ssl_session.hpp>

using boost::asio::ip::tcp;

namespace foxhttp {

ssl_server::ssl_server(io_context_pool &io_pool, unsigned short port, boost::asio::ssl::context &ssl_ctx)
    : io_pool_(io_pool), acceptor_(io_pool.get_io_context(), tcp::endpoint(tcp::v4(), port)), ssl_ctx_(ssl_ctx),
      global_chain_(std::make_shared<middleware_chain>(io_pool.get_io_context()))
{
    _do_accept();
}

void ssl_server::use(std::shared_ptr<middleware_chain> chain)
{
    global_chain_ = std::move(chain);
}

std::shared_ptr<middleware_chain> ssl_server::global_chain() const
{
    return global_chain_;
}

void ssl_server::_do_accept()
{
    auto &io = io_pool_.get_io_context();
    auto socket = tcp::socket(io);
    acceptor_.async_accept(socket, [this, s = std::move(socket)](const boost::system::error_code &ec) mutable {
        if (!ec)
        {
            auto stream = boost::asio::ssl::stream<tcp::socket>(std::move(s), ssl_ctx_);
            std::make_shared<ssl_session>(std::move(stream), global_chain_)->start();
        }
        _do_accept();
    });
}

}// namespace foxhttp
