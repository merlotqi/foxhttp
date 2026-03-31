#include <spdlog/spdlog.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/io_context_pool.hpp>
#include <foxhttp/server/server.hpp>
#include <foxhttp/server/session.hpp>

namespace foxhttp {

server::server(io_context_pool& io_pool, unsigned short port)
    : io_pool_(io_pool),
      listen_io_(&io_pool_.get_io_context()),
      acceptor_(*listen_io_, {tcp::v4(), port}),
      global_chain_(std::make_shared<middleware_chain>(*listen_io_)) {
  do_accept();
}

void server::use(std::shared_ptr<middleware> mw) { global_chain_->use(std::move(mw)); }

std::shared_ptr<middleware_chain> server::global_chain() const { return global_chain_; }

void server::stop() {
  stopping_.store(true, std::memory_order_release);
  try {
    acceptor_.close();
  } catch (const boost::system::system_error& e) {
    spdlog::warn("server::stop: failed to close acceptor: {}", e.what());
  }
}

void server::do_accept() {
  boost::asio::co_spawn(
      acceptor_.get_executor(), [this]() -> boost::asio::awaitable<void> { co_await accept_loop(); },
      boost::asio::detached);
}

boost::asio::awaitable<void> server::accept_loop() {
  namespace asio = boost::asio;
  while (!stopping_.load(std::memory_order_acquire)) {
    boost::system::error_code ec;
    tcp::socket sock(*listen_io_);
    co_await acceptor_.async_accept(sock, asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
      if (ec == asio::error::operation_aborted) {
        co_return;
      }
      spdlog::warn("server::accept_loop: accept error: {}", ec.message());
      continue;
    }
    
    auto stopping = stopping_.load(std::memory_order_acquire);
    if (!stopping) {
      std::make_shared<session>(std::move(sock), global_chain_)->start();
    }
  }
}

}  // namespace foxhttp
