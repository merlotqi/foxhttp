/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/bind/bind.hpp>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

namespace foxhttp {

class MiddlewareChain;
class Session : public std::enable_shared_from_this<Session>
{
public:
    explicit Session(tcp::socket socket, std::shared_ptr<MiddlewareChain> global_chain);
    void start();

private:
    void read_request();
    void handle_read(boost::beast::error_code ec, size_t bytes_transferred);
    void process_request();
    void write_response();

private:
    tcp::socket socket_;
    boost::beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    http::response<http::string_body> res_;
    std::shared_ptr<MiddlewareChain> global_chain_;
};

}// namespace foxhttp