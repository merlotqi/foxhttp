#include <spdlog/spdlog.h>

#include <foxhttp/config.hpp>
#include <foxhttp/core/error_codes.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/request_context.hpp>
#include <foxhttp/server/session.hpp>
#include <foxhttp/server/ws_session.hpp>

#if FOXHTTP_HAS_COROUTINES
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <foxhttp/detail/await_middleware_async.hpp>
#endif

#include <boost/beast/websocket.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;

namespace foxhttp::server {

Session::Session(tcp::socket socket, std::shared_ptr<MiddlewareChain> global_chain)
    : SessionBase(socket.get_executor(), global_chain),
      socket_(std::move(socket)),
      global_chain_(std::move(global_chain)) {}

void Session::start() {
  arm_idle_timer();
#if FOXHTTP_HAS_COROUTINES
  asio::co_spawn(
      socket_.get_executor(), [self = shared_from_this()]() -> asio::awaitable<void> { co_await self->run(); },
      asio::detached);
#else
  run();
#endif
}

#if FOXHTTP_HAS_COROUTINES
asio::awaitable<void> Session::run() {
  try {
    while (true) {
      arm_header_timer();
      read_abort_ = ReadAbortReason::None;

      beast::error_code ec;
      co_await http::async_read(socket_, buffer_, req_, asio::redirect_error(asio::use_awaitable, ec));
      if (ec) {
        cancel_header_timer();
        if (ec == asio::error::operation_aborted) {
          const auto why = read_abort_.exchange(ReadAbortReason::None);
          if (why == ReadAbortReason::HeaderTimeout) {
            res_ = http::response<http::string_body>{http::status::request_timeout, req_.version()};
            res_.set(http::field::content_type, "text/plain");
            res_.body() = "Header read timeout";
            res_.prepare_payload();
            co_await http::async_write(socket_, res_, asio::redirect_error(asio::use_awaitable, ec));
          } else if (why == ReadAbortReason::BodyTimeout) {
            res_ = http::response<http::string_body>{http::status::request_timeout, req_.version()};
            res_.set(http::field::content_type, "text/plain");
            res_.body() = "Body read timeout";
            res_.prepare_payload();
            co_await http::async_write(socket_, res_, asio::redirect_error(asio::use_awaitable, ec));
          }
        } else if (ec != http::error::end_of_stream && ec != asio::error::operation_aborted) {
          spdlog::warn("foxhttp session read error: {}", ec.message());
        }
        co_return;
      }

      cancel_header_timer();
      on_activity();

      if (beast::websocket::is_upgrade(req_)) {
        beast::tcp_stream ts(std::move(socket_));
        beast::websocket::stream<beast::tcp_stream> ws(std::move(ts));
        auto ws_sess = std::make_shared<WsSession>(std::move(ws));
        ws_sess->start_accept(std::move(req_));

        cancel_idle_timer();
        cancel_header_timer();
        cancel_body_timer();

        co_return;
      }

      {
        RequestContext ctx(req_);
        co_await detail::await_middleware_chain_async(socket_.get_executor(), global_chain_, ctx, res_);
      }

      res_.prepare_payload();
      co_await http::async_write(socket_, res_, asio::redirect_error(asio::use_awaitable, ec));
      if (ec) {
        if (ec != asio::error::operation_aborted) {
          spdlog::warn("foxhttp session write error: {}", ec.message());
        }
        try {
          socket_.shutdown(tcp::socket::shutdown_both);
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
          socket_.shutdown(tcp::socket::shutdown_both);
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
    spdlog::warn("foxhttp session coroutine: {}", e.what());
    notify_error(std::current_exception());
  } catch (...) {
    spdlog::warn("foxhttp session coroutine: unknown exception");
    notify_error(std::current_exception());
  }
}
#else
void Session::run() {
  // C++17 callback-based implementation
  auto self = shared_from_this();
  
  auto do_read = [this, self]() {
    arm_header_timer();
    read_abort_ = ReadAbortReason::None;

    auto read_callback = [this, self](beast::error_code ec, std::size_t /*bytes_transferred*/) {
      cancel_header_timer();
      
      if (ec) {
        if (ec == asio::error::operation_aborted) {
          const auto why = read_abort_.exchange(ReadAbortReason::None);
          if (why == ReadAbortReason::HeaderTimeout) {
            res_ = http::response<http::string_body>{http::status::request_timeout, req_.version()};
            res_.set(http::field::content_type, "text/plain");
            res_.body() = "Header read timeout";
            res_.prepare_payload();
            
            http::async_write(socket_, res_, [this, self](beast::error_code, std::size_t) {
              // Connection closed after timeout response
            });
          } else if (why == ReadAbortReason::BodyTimeout) {
            res_ = http::response<http::string_body>{http::status::request_timeout, req_.version()};
            res_.set(http::field::content_type, "text/plain");
            res_.body() = "Body read timeout";
            res_.prepare_payload();
            
            http::async_write(socket_, res_, [this, self](beast::error_code, std::size_t) {
              // Connection closed after timeout response
            });
          }
        } else if (ec != http::error::end_of_stream) {
          spdlog::warn("foxhttp session read error: {}", ec.message());
        }
        return;
      }

      on_activity();

      if (beast::websocket::is_upgrade(req_)) {
        beast::tcp_stream ts(std::move(socket_));
        beast::websocket::stream<beast::tcp_stream> ws(std::move(ts));
        auto ws_sess = std::make_shared<WsSession>(std::move(ws));
        ws_sess->start_accept(std::move(req_));

        cancel_idle_timer();
        cancel_header_timer();
        cancel_body_timer();
        return;
      }

      // Execute middleware chain and then write response
      {
        RequestContext ctx(req_);
        global_chain_->execute_async(ctx, res_, [this, self](MiddlewareResult, const std::string &) {
          // Write response
          res_.prepare_payload();
          http::async_write(socket_, res_, [this, self](beast::error_code ec, std::size_t /*bytes*/) {
            if (ec) {
              if (ec != asio::error::operation_aborted) {
                spdlog::warn("foxhttp session write error: {}", ec.message());
              }
              try {
                socket_.shutdown(tcp::socket::shutdown_both);
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
                socket_.shutdown(tcp::socket::shutdown_both);
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
            do_read();
          });
        });
      }
    };

    http::async_read(socket_, buffer_, req_, read_callback);
  };

  do_read();
}
#endif

void Session::on_timeout_idle() {
  try {
    socket_.shutdown(tcp::socket::shutdown_both);
  } catch (const boost::system::system_error &e) {
    spdlog::warn("Failed to shutdown socket on idle timeout: {}", e.what());
  }
}

void Session::on_timeout_header() {
  read_abort_ = ReadAbortReason::HeaderTimeout;
  beast::get_lowest_layer(socket_).cancel();
}

void Session::on_timeout_body() {
  read_abort_ = ReadAbortReason::BodyTimeout;
  beast::get_lowest_layer(socket_).cancel();
}

}  // namespace foxhttp::server
