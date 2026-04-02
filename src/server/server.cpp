#include <spdlog/spdlog.h>

#include <foxhttp/config.hpp>
#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/io_context_pool.hpp>
#include <foxhttp/server/server.hpp>
#include <foxhttp/server/session.hpp>

#if FOXHTTP_HAS_COROUTINES
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#endif

namespace foxhttp::server {

Server::Server(IoContextPool& io_pool, unsigned short port)
    : io_pool_(io_pool),
      listen_io_(&io_pool_.get_io_context()),
      acceptor_(*listen_io_, {tcp::v4(), port}),
      global_chain_(std::make_shared<MiddlewareChain>(*listen_io_)) {
  do_accept();
}

void Server::use(std::shared_ptr<Middleware> mw) { global_chain_->use(std::move(mw)); }

std::shared_ptr<MiddlewareChain> Server::global_chain() const { return global_chain_; }

void Server::stop() {
  stopping_.store(true, std::memory_order_release);
  try {
    acceptor_.close();
  } catch (const boost::system::system_error& e) {
    spdlog::warn("Server::stop: failed to close acceptor: {}", e.what());
  }
}

void Server::do_accept() {
#if FOXHTTP_HAS_COROUTINES
  boost::asio::co_spawn(
      acceptor_.get_executor(), [this]() -> boost::asio::awaitable<void> { co_await accept_loop(); },
      boost::asio::detached);
#else
  accept_loop();
#endif
}

#if FOXHTTP_HAS_COROUTINES
boost::asio::awaitable<void> Server::accept_loop() {
  namespace asio = boost::asio;
  while (!stopping_.load(std::memory_order_acquire)) {
    boost::system::error_code ec;
    tcp::socket sock(*listen_io_);
    co_await acceptor_.async_accept(sock, asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
      if (ec == asio::error::operation_aborted) {
        co_return;
      }
      spdlog::warn("Server::accept_loop: accept error: {}", ec.message());
      continue;
    }

    auto stopping = stopping_.load(std::memory_order_acquire);
    if (!stopping) {
      std::make_shared<Session>(std::move(sock), global_chain_)->start();
    }
  }
}
#else
void Server::accept_loop() {
  if (stopping_.load(std::memory_order_acquire)) return;

  auto sock = std::make_shared<tcp::socket>(*listen_io_);
  namespace asio = boost::asio;
  acceptor_.async_accept(*sock, [this, sock](boost::system::error_code ec) {
    if (ec) {
      if (ec == asio::error::operation_aborted) {
        return;
      }
      spdlog::warn("Server::accept_loop: accept error: {}", ec.message());
      // Continue accepting
      accept_loop();
      return;
    }

    if (!stopping_.load(std::memory_order_acquire)) {
      std::make_shared<Session>(std::move(*sock), global_chain_)->start();
    }
    // Continue accepting
    accept_loop();
  });
}
#endif

}  // namespace foxhttp::server
