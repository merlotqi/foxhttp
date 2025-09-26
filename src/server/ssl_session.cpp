/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * middleware utilities and helper classes
 */

#include <boost/asio/io_context.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/request_context.hpp>
#include <foxhttp/server/ssl_session.hpp>
#if USING_TLS
#include <foxhttp/server/wss_session.hpp>
#endif

namespace beast = boost::beast;
namespace http = beast::http;

namespace foxhttp {

ssl_session::ssl_session(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream,
                         std::shared_ptr<middleware_chain> global_chain)
    : session_base(stream.get_executor(), global_chain), stream_(std::move(stream)),
      global_chain_(std::move(global_chain))
{
}

void ssl_session::start()
{
    arm_idle_timer();
    _handshake();
}

void ssl_session::_handshake()
{
    auto self = shared_from_this();
    stream_.async_handshake(boost::asio::ssl::stream_base::server, [self](boost::system::error_code ec) {
        if (!ec)
        {
            self->on_activity();
            self->arm_header_timer();
            self->_read_request();
        }
    });
}

void ssl_session::_read_request()
{
    auto self = shared_from_this();
    http::async_read(stream_, buffer_, req_, [self](beast::error_code ec, std::size_t bytes_transferred) {
        self->_handle_read(ec, bytes_transferred);
    });
}

void ssl_session::_handle_read(beast::error_code ec, std::size_t)
{
    if (ec)
    {
        return;
    }
    cancel_header_timer();
    on_activity();
    _process_request();
}

void ssl_session::_process_request()
{
    auto self = shared_from_this();
    request_context ctx(req_);

#ifdef USING_TLS

    // WebSocket upgrade detection (wss)
    if (boost::beast::websocket::is_upgrade(req_))
    {
        using tls_stream = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
        boost::beast::websocket::stream<tls_stream> ws(std::move(stream_));
        auto wss_sess = std::make_shared<wss_session>(std::move(ws));
        wss_sess->start_accept(req_);
        return;
    }
#endif
    global_chain_->execute_async(ctx, res_,
                                 [self](middleware_result, const std::string &) { self->_write_response(); });
}

void ssl_session::_write_response()
{
    auto self = shared_from_this();
    res_.prepare_payload();
    http::async_write(stream_, res_, [self](beast::error_code, std::size_t) {
        beast::error_code ec;
        self->stream_.shutdown(ec);
    });
}

void ssl_session::on_timeout_idle()
{
    beast::error_code ec;
    stream_.next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
}

void ssl_session::on_timeout_header()

{
    // respond 408 Request Timeout
    res_.result(http::status::request_timeout);
    res_.set(http::field::content_type, "text/plain");
    res_.body() = "Header read timeout";
    _write_response();
}

void ssl_session::on_timeout_body()
{
    res_.result(http::status::request_timeout);
    res_.set(http::field::content_type, "text/plain");
    res_.body() = "Body read timeout";
    _write_response();
}

}// namespace foxhttp
