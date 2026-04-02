#include <spdlog/spdlog.h>

#include <foxhttp/config.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/io_context_pool.hpp>
#include <foxhttp/server/ssl_server.hpp>
#include <foxhttp/server/ssl_session.hpp>

#if FOXHTTP_HAS_COROUTINES
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#endif

using boost::asio::ip::tcp;

namespace foxhttp::server {

SslServer::SslServer(IoContextPool &io_pool, unsigned short port, boost::asio::ssl::context &ssl_ctx)
    : io_pool_(io_pool),
      listen_io_(&io_pool.get_io_context()),
      acceptor_(*listen_io_, tcp::endpoint(tcp::v4(), port)),
      ssl_ctx_(ssl_ctx),
      global_chain_(std::make_shared<MiddlewareChain>(*listen_io_)) {
  do_accept();
}

void SslServer::use(std::shared_ptr<MiddlewareChain> chain) { global_chain_ = std::move(chain); }

std::shared_ptr<MiddlewareChain> SslServer::global_chain() const { return global_chain_; }

void SslServer::stop() {
  stopping_.store(true, std::memory_order_release);
  try {
    acceptor_.close();
  } catch (const boost::system::system_error &e) {
    spdlog::warn("SslServer::stop: failed to close acceptor: {}", e.what());
  }
}

void SslServer::do_accept() {
#if FOXHTTP_HAS_COROUTINES
  boost::asio::co_spawn(
      acceptor_.get_executor(), [this]() -> boost::asio::awaitable<void> { co_await accept_loop(); },
      boost::asio::detached);
#else
  accept_loop();
#endif
}

#if FOXHTTP_HAS_COROUTINES
boost::asio::awaitable<void> SslServer::accept_loop() {
  namespace asio = boost::asio;
  while (!stopping_.load(std::memory_order_acquire)) {
    boost::system::error_code ec;
    tcp::socket sock(*listen_io_);
    co_await acceptor_.async_accept(sock, asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
      if (ec == asio::error::operation_aborted && stopping_.load(std::memory_order_acquire)) {
        co_return;
      }
      if (ec != asio::error::operation_aborted) {
        spdlog::warn("SslServer::accept_loop: accept error: {}", ec.message());
      }
      continue;
    }
    if (!stopping_.load(std::memory_order_acquire)) {
      auto stream = asio::ssl::stream<tcp::socket>(std::move(sock), ssl_ctx_);
      std::make_shared<SslSession>(std::move(stream), global_chain_)->start();
    }
  }
}
#else
void SslServer::accept_loop() {
  if (stopping_.load(std::memory_order_acquire)) return;

  auto sock = std::make_shared<tcp::socket>(*listen_io_);
  namespace asio = boost::asio;
  acceptor_.async_accept(*sock, [this, sock](boost::system::error_code ec) {
    if (ec) {
      if (ec == asio::error::operation_aborted && stopping_.load(std::memory_order_acquire)) {
        return;
      }
      if (ec != asio::error::operation_aborted) {
        spdlog::warn("SslServer::accept_loop: accept error: {}", ec.message());
      }
      // Continue accepting
      accept_loop();
      return;
    }
    if (!stopping_.load(std::memory_order_acquire)) {
      auto stream = asio::ssl::stream<tcp::socket>(std::move(*sock), ssl_ctx_);
      std::make_shared<SslSession>(std::move(stream), global_chain_)->start();
    }
    // Continue accepting
    accept_loop();
  });
}
#endif

}  // namespace foxhttp::server
