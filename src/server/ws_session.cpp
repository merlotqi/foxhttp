#include <foxhttp/server/ws_session.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;

namespace foxhttp {

ws_session::ws_session(websocket_t ws, const websocket_limits &wsl, const session_limits &sl)
    : session_base(ws.get_executor(), nullptr, sl), ws_(std::move(ws)), wsl_(wsl) {
  ws_.read_message_max(wsl_.max_message_bytes);
}

void ws_session::start_accept(http::request<http::string_body> req) {
  arm_idle_timer();
  ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
  ws_.async_accept(req, [self = shared_from_this()](boost::system::error_code ec) {
    static_cast<ws_session *>(self.get())->on_accept(ec);
  });
}

void ws_session::on_accept(boost::system::error_code ec) {
  if (ec) return;
  do_read();
}

void ws_session::do_read() {
  ws_.async_read(buffer_, [self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) {
    static_cast<ws_session *>(self.get())->on_read(ec, bytes_transferred);
  });
}

void ws_session::on_read(boost::system::error_code ec, std::size_t) {
  if (ec) return;
  // Echo
  ws_.text(ws_.got_text());
  ws_.async_write(buffer_.data(), [self = shared_from_this()](boost::system::error_code, std::size_t) {
    auto *s = static_cast<ws_session *>(self.get());
    s->buffer_.consume(s->buffer_.size());
    s->do_read();
  });
}

void ws_session::do_write() {}

void ws_session::on_timeout_idle() {
  beast::error_code ec;
  ws_.next_layer().socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
}

}  // namespace foxhttp
