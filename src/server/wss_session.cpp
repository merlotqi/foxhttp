/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * middleware utilities and helper classes
 */

#include <boost/beast/websocket/ssl.hpp>
#include <foxhttp/server/wss_session.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;

namespace foxhttp {

wss_session::wss_session(websocket_t ws, const websocket_limits &wsl, const session_limits &sl)
    : session_base(ws.get_executor(), nullptr, sl), ws_(std::move(ws)), wsl_(wsl)
{
    ws_.read_message_max(wsl_.max_message_bytes);
}

void wss_session::start_accept(http::request<http::string_body> req)
{
    arm_idle_timer();
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    ws_.async_accept(req, [self = shared_from_this()](boost::system::error_code ec) {
        static_cast<wss_session *>(self.get())->_on_accept(ec);
    });
}

void wss_session::_on_accept(boost::system::error_code ec)
{
    if (ec)
        return;
    _do_read();
}

void wss_session::_do_read()
{
    ws_.async_read(buffer_, [self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) {
        static_cast<wss_session *>(self.get())->_on_read(ec, bytes_transferred);
    });
}

void wss_session::_on_read(boost::system::error_code ec, std::size_t)
{
    if (ec)
        return;
    ws_.text(ws_.got_text());
    ws_.async_write(buffer_.data(), [self = shared_from_this()](boost::system::error_code, std::size_t) {
        auto *s = static_cast<wss_session *>(self.get());
        s->buffer_.consume(s->buffer_.size());
        s->_do_read();
    });
}

void wss_session::on_timeout_idle()
{
    beast::error_code ec;
    ws_.next_layer().next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
}

}// namespace foxhttp
