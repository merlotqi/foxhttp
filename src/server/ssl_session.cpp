#include <foxhttp/detail/await_middleware_async.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/request_context.hpp>
#include <foxhttp/server/ssl_session.hpp>

#if USING_TLS
#include <foxhttp/server/wss_session.hpp>
#endif

#include <spdlog/spdlog.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/websocket.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;

namespace foxhttp {

ssl_session::ssl_session(asio::ssl::stream<asio::ip::tcp::socket> stream,
                         std::shared_ptr<middleware_chain> global_chain)
    : session_base(stream.get_executor(), global_chain),
      stream_(std::move(stream)),
      global_chain_(std::move(global_chain)),
      handshake_timer_(stream_.get_executor()) {}

void ssl_session::start() {
  arm_idle_timer();
  asio::co_spawn(
      stream_.get_executor(), [self = shared_from_this()]() -> asio::awaitable<void> { co_await self->run(); },
      asio::detached);
}

asio::awaitable<void> ssl_session::run() {
  try {
    beast::error_code ec;
    arm_handshake_timer();
    co_await stream_.async_handshake(asio::ssl::stream_base::server, asio::redirect_error(asio::use_awaitable, ec));
    cancel_handshake_timer();
      if (ec) {
        if (ec == asio::error::operation_aborted && read_abort_ == read_abort_reason::handshake_timeout) {
          spdlog::warn("foxhttp ssl_session handshake timeout");
          try {
            stream_.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both);
          } catch (const boost::system::system_error &e) {
            spdlog::warn("Failed to shutdown socket after handshake timeout: {}", e.what());
          }
          co_return;
        }
        if (ec != asio::error::operation_aborted) {
          spdlog::warn("foxhttp ssl_session handshake error: {}", ec.message());
        }
        co_return;
      }
    on_activity();

    while (true) {
      arm_header_timer();
      read_abort_ = read_abort_reason::none;

      co_await http::async_read(stream_, buffer_, req_, asio::redirect_error(asio::use_awaitable, ec));
      if (ec) {
        cancel_header_timer();
        if (ec == asio::error::operation_aborted) {
          const auto why = read_abort_.exchange(read_abort_reason::none);
          if (why == read_abort_reason::header_timeout) {
            res_ = http::response<http::string_body>{http::status::request_timeout, req_.version()};
            res_.set(http::field::content_type, "text/plain");
            res_.body() = "Header read timeout";
            res_.prepare_payload();
            co_await http::async_write(stream_, res_, asio::redirect_error(asio::use_awaitable, ec));
          } else if (why == read_abort_reason::body_timeout) {
            res_ = http::response<http::string_body>{http::status::request_timeout, req_.version()};
            res_.set(http::field::content_type, "text/plain");
            res_.body() = "Body read timeout";
            res_.prepare_payload();
            co_await http::async_write(stream_, res_, asio::redirect_error(asio::use_awaitable, ec));
          }
        } else if (ec != http::error::end_of_stream && ec != asio::error::operation_aborted) {
          spdlog::warn("foxhttp ssl_session read error: {}", ec.message());
        }
        co_return;
      }

      cancel_header_timer();
      on_activity();

#if USING_TLS
      if (beast::websocket::is_upgrade(req_)) {
        using tls_stream = asio::ssl::stream<asio::ip::tcp::socket>;
        beast::websocket::stream<tls_stream> ws(std::move(stream_));
        auto wss_sess = std::make_shared<wss_session>(std::move(ws));
        wss_sess->start_accept(std::move(req_));
        co_return;
      }
#endif

      {
        request_context ctx(req_);
        co_await detail::await_middleware_chain_async(stream_.get_executor(), global_chain_, ctx, res_);
      }

      res_.prepare_payload();
      co_await http::async_write(stream_, res_, asio::redirect_error(asio::use_awaitable, ec));
      if (ec) {
        if (ec != asio::error::operation_aborted) {
          spdlog::warn("foxhttp ssl_session write error: {}", ec.message());
        }
        try {
          stream_.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both);
        } catch (const boost::system::system_error &e) {
          spdlog::warn("Failed to shutdown socket after write error: {}", e.what());
        }
        co_return;
      }

      ++requests_served_;
      const auto &lim = limits();
      const bool keep_open =
          lim.enable_keep_alive && res_.keep_alive() &&
          (lim.max_requests_per_connection == 0 || requests_served_ < lim.max_requests_per_connection);
      if (!keep_open) {
        try {
          stream_.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both);
        } catch (const boost::system::system_error &e) {
          spdlog::warn("Failed to shutdown socket on connection close: {}", e.what());
        }
        co_return;
      }

      const unsigned req_ver = req_.version();
      req_ = {};
      res_ = http::response<http::string_body>{http::status::ok, req_ver};
      on_activity();
    }
  } catch (const std::exception &e) {
    spdlog::warn("foxhttp ssl_session coroutine: {}", e.what());
    notify_error(std::current_exception());
  } catch (...) {
    spdlog::warn("foxhttp ssl_session coroutine: unknown exception");
    notify_error(std::current_exception());
  }
}

void ssl_session::on_timeout_idle() {
  try {
    stream_.next_layer().shutdown(asio::ip::tcp::socket::shutdown_both);
  } catch (const boost::system::system_error &e) {
    spdlog::warn("Failed to shutdown socket on idle timeout: {}", e.what());
  }
}

void ssl_session::on_timeout_header() {
  read_abort_ = read_abort_reason::header_timeout;
  beast::get_lowest_layer(stream_).cancel();
}

void ssl_session::on_timeout_body() {
  read_abort_ = read_abort_reason::body_timeout;
  beast::get_lowest_layer(stream_).cancel();
}

void ssl_session::arm_handshake_timer() {
  handshake_timer_.expires_after(std::chrono::seconds(30));
  handshake_timer_.async_wait([this](const boost::system::error_code &ec) {
    if (!ec) {
      read_abort_ = read_abort_reason::handshake_timeout;
      beast::get_lowest_layer(stream_).cancel();
    }
  });
}

void ssl_session::cancel_handshake_timer() {
  handshake_timer_.cancel();
}

}  // namespace foxhttp
