#include <foxhttp/config.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/request_context.hpp>
#include <foxhttp/server/ssl_session.hpp>

#if FOXHTTP_HAS_COROUTINES
#include <foxhttp/detail/await_middleware_async.hpp>
#endif

#if USING_TLS
#include <foxhttp/server/wss_session.hpp>
#endif

#include <spdlog/spdlog.h>

#if FOXHTTP_HAS_COROUTINES
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#endif
#include <boost/beast/websocket.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;

namespace foxhttp::server {

SslSession::SslSession(asio::ssl::stream<asio::ip::tcp::socket> stream,
                         std::shared_ptr<MiddlewareChain> global_chain)
    : SessionBase(stream.get_executor(), global_chain),
      stream_(std::move(stream)),
      global_chain_(std::move(global_chain)),
      handshake_timer_(stream_.get_executor()) {}

void SslSession::start() {
  arm_idle_timer();
#if FOXHTTP_HAS_COROUTINES
  asio::co_spawn(
      stream_.get_executor(), [self = shared_from_this()]() -> asio::awaitable<void> { co_await self->run(); },
      asio::detached);
#else
  run();
#endif
}

#if FOXHTTP_HAS_COROUTINES
asio::awaitable<void> SslSession::run() {
  try {
    beast::error_code ec;
    arm_handshake_timer();
    co_await stream_.async_handshake(asio::ssl::stream_base::server, asio::redirect_error(asio::use_awaitable, ec));
    cancel_handshake_timer();
    if (ec) {
      if (ec == asio::error::operation_aborted && read_abort_ == ReadAbortReason::HandshakeTimeout) {
        spdlog::warn("foxhttp SslSession handshake timeout");
        try {
          stream_.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both);
        } catch (const boost::system::system_error &e) {
          spdlog::warn("Failed to shutdown socket after handshake timeout: {}", e.what());
        }
        co_return;
      }
      if (ec != asio::error::operation_aborted) {
        spdlog::warn("foxhttp SslSession handshake error: {}", ec.message());
      }
      co_return;
    }
    on_activity();

    while (true) {
      arm_header_timer();
      read_abort_ = ReadAbortReason::None;

      co_await http::async_read(stream_, buffer_, req_, asio::redirect_error(asio::use_awaitable, ec));
      if (ec) {
        cancel_header_timer();
        if (ec == asio::error::operation_aborted) {
          const auto why = read_abort_.exchange(ReadAbortReason::None);
          if (why == ReadAbortReason::HeaderTimeout) {
            res_ = http::response<http::string_body>{http::status::request_timeout, req_.version()};
            res_.set(http::field::content_type, "text/plain");
            res_.body() = "Header read timeout";
            res_.prepare_payload();
            co_await http::async_write(stream_, res_, asio::redirect_error(asio::use_awaitable, ec));
          } else if (why == ReadAbortReason::BodyTimeout) {
            res_ = http::response<http::string_body>{http::status::request_timeout, req_.version()};
            res_.set(http::field::content_type, "text/plain");
            res_.body() = "Body read timeout";
            res_.prepare_payload();
            co_await http::async_write(stream_, res_, asio::redirect_error(asio::use_awaitable, ec));
          }
        } else if (ec != http::error::end_of_stream && ec != asio::error::operation_aborted) {
          spdlog::warn("foxhttp SslSession read error: {}", ec.message());
        }
        co_return;
      }

      cancel_header_timer();
      on_activity();

#if USING_TLS
      if (beast::websocket::is_upgrade(req_)) {
        using tls_stream = asio::ssl::stream<asio::ip::tcp::socket>;
        beast::websocket::stream<tls_stream> ws(std::move(stream_));
        auto wss_sess = std::make_shared<WssSession>(std::move(ws));
        wss_sess->start_accept(std::move(req_));
        co_return;
      }
#endif

      {
        RequestContext ctx(req_);
        co_await detail::await_middleware_chain_async(stream_.get_executor(), global_chain_, ctx, res_);
      }

      res_.prepare_payload();
      co_await http::async_write(stream_, res_, asio::redirect_error(asio::use_awaitable, ec));
      if (ec) {
        if (ec != asio::error::operation_aborted) {
          spdlog::warn("foxhttp SslSession write error: {}", ec.message());
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
          spdlog::warn("Failed to shutdown socket on Connection close: {}", e.what());
        }
        co_return;
      }

      const unsigned req_ver = req_.version();
      req_ = {};
      res_ = http::response<http::string_body>{http::status::ok, req_ver};
      on_activity();
    }
  } catch (const std::exception &e) {
    spdlog::warn("foxhttp SslSession coroutine: {}", e.what());
    notify_error(std::current_exception());
  } catch (...) {
    spdlog::warn("foxhttp SslSession coroutine: unknown exception");
    notify_error(std::current_exception());
  }
}
#else
void SslSession::run() {
  auto self = shared_from_this();
  
  // Phase 1: TLS handshake
  arm_handshake_timer();
  stream_.async_handshake(asio::ssl::stream_base::server, [this, self](beast::error_code ec) {
    cancel_handshake_timer();
    
    if (ec) {
      if (ec == asio::error::operation_aborted && read_abort_ == ReadAbortReason::HandshakeTimeout) {
        spdlog::warn("foxhttp SslSession handshake timeout");
        try {
          stream_.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both);
        } catch (const boost::system::system_error &e) {
          spdlog::warn("Failed to shutdown socket after handshake timeout: {}", e.what());
        }
      } else if (ec != asio::error::operation_aborted) {
        spdlog::warn("foxhttp SslSession handshake error: {}", ec.message());
      }
      return;
    }
    on_activity();
    
    // Phase 2: Start reading requests (callback loop)
    do_read_request();
  });
}

void SslSession::do_read_request() {
  auto self = shared_from_this();
  
  arm_header_timer();
  read_abort_ = ReadAbortReason::None;

  http::async_read(stream_, buffer_, req_, [this, self](beast::error_code ec, std::size_t /*bytes*/) {
    cancel_header_timer();
    
    if (ec) {
      if (ec == asio::error::operation_aborted) {
        const auto why = read_abort_.exchange(ReadAbortReason::None);
        if (why == ReadAbortReason::HeaderTimeout) {
          res_ = http::response<http::string_body>{http::status::request_timeout, req_.version()};
          res_.set(http::field::content_type, "text/plain");
          res_.body() = "Header read timeout";
          res_.prepare_payload();
          http::async_write(stream_, res_, [this, self](beast::error_code, std::size_t) {});
        } else if (why == ReadAbortReason::BodyTimeout) {
          res_ = http::response<http::string_body>{http::status::request_timeout, req_.version()};
          res_.set(http::field::content_type, "text/plain");
          res_.body() = "Body read timeout";
          res_.prepare_payload();
          http::async_write(stream_, res_, [this, self](beast::error_code, std::size_t) {});
        }
      } else if (ec != http::error::end_of_stream) {
        spdlog::warn("foxhttp SslSession read error: {}", ec.message());
      }
      return;
    }

    on_activity();

#if USING_TLS
    if (beast::websocket::is_upgrade(req_)) {
      using tls_stream = asio::ssl::stream<asio::ip::tcp::socket>;
      beast::websocket::stream<tls_stream> ws(std::move(stream_));
      auto wss_sess = std::make_shared<WssSession>(std::move(ws));
      wss_sess->start_accept(std::move(req_));
      return;
    }
#endif

    // Execute middleware chain
    {
      RequestContext ctx(req_);
      global_chain_->execute_async(ctx, res_, [this, self](MiddlewareResult, const std::string &) {
        res_.prepare_payload();
        http::async_write(stream_, res_, [this, self](beast::error_code ec, std::size_t /*bytes*/) {
          if (ec) {
            if (ec != asio::error::operation_aborted) {
              spdlog::warn("foxhttp SslSession write error: {}", ec.message());
            }
            try {
              stream_.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both);
            } catch (const boost::system::system_error &e) {
              spdlog::warn("Failed to shutdown socket after write error: {}", e.what());
            }
            return;
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
              spdlog::warn("Failed to shutdown socket on Connection close: {}", e.what());
            }
            return;
          }

          const unsigned req_ver = req_.version();
          req_ = {};
          res_ = http::response<http::string_body>{http::status::ok, req_ver};
          on_activity();
          
          // Continue to next request
          do_read_request();
        });
      });
    }
  });
}
#endif

void SslSession::on_timeout_idle() {
  try {
    stream_.next_layer().shutdown(asio::ip::tcp::socket::shutdown_both);
  } catch (const boost::system::system_error &e) {
    spdlog::warn("Failed to shutdown socket on idle timeout: {}", e.what());
  }
}

void SslSession::on_timeout_header() {
  read_abort_ = ReadAbortReason::HeaderTimeout;
  beast::get_lowest_layer(stream_).cancel();
}

void SslSession::on_timeout_body() {
  read_abort_ = ReadAbortReason::BodyTimeout;
  beast::get_lowest_layer(stream_).cancel();
}

void SslSession::arm_handshake_timer() {
  handshake_timer_.expires_after(std::chrono::seconds(30));
  handshake_timer_.async_wait([this](const boost::system::error_code &ec) {
    if (!ec) {
      read_abort_ = ReadAbortReason::HandshakeTimeout;
      beast::get_lowest_layer(stream_).cancel();
    }
  });
}

void SslSession::cancel_handshake_timer() { handshake_timer_.cancel(); }

}  // namespace foxhttp::server
