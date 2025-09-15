/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
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

class middleware_chain;
class session : public std::enable_shared_from_this<session>
{
public:
    explicit session(tcp::socket socket, std::shared_ptr<middleware_chain> global_chain);
    void start();

private:
    void _read_request();
    void _handle_read(boost::beast::error_code ec, size_t bytes_transferred);
    void _process_request();
    void _write_response();

private:
    tcp::socket socket_;
    boost::beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    http::response<http::string_body> res_;
    std::shared_ptr<middleware_chain> global_chain_;
};

}// namespace foxhttp