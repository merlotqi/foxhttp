/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace foxhttp {

class Middleware;
class IOContextPool;
class MiddlewareChain;
class Server
{
public:
    Server(IOContextPool &io_pool, unsigned short port);

    void use(std::shared_ptr<Middleware> mw);
    std::shared_ptr<MiddlewareChain> global_chain() const;

private:
    void do_accept();

    IOContextPool &io_pool_;
    tcp::acceptor acceptor_;
    std::shared_ptr<MiddlewareChain> global_chain_;
};

}// namespace foxhttp