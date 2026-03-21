#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/io_context_pool.hpp>
#include <foxhttp/server/ssl_server.hpp>
#include <foxhttp/server/ssl_session.hpp>

using boost::asio::ip::tcp;

namespace foxhttp {

ssl_server::ssl_server(io_context_pool &io_pool, unsigned short port, boost::asio::ssl::context &ssl_ctx)
    : io_pool_(io_pool),
      listen_io_(&io_pool.get_io_context()),
      acceptor_(*listen_io_, tcp::endpoint(tcp::v4(), port)),
      ssl_ctx_(ssl_ctx),
      global_chain_(std::make_shared<middleware_chain>(*listen_io_)) {
  do_accept();
}

void ssl_server::use(std::shared_ptr<middleware_chain> chain) { global_chain_ = std::move(chain); }

std::shared_ptr<middleware_chain> ssl_server::global_chain() const { return global_chain_; }

void ssl_server::do_accept() {
  boost::asio::co_spawn(
      acceptor_.get_executor(), [this]() -> boost::asio::awaitable<void> { co_await accept_loop(); },
      boost::asio::detached);
}

boost::asio::awaitable<void> ssl_server::accept_loop() {
  namespace asio = boost::asio;
  while (true) {
    boost::system::error_code ec;
    tcp::socket sock(*listen_io_);
    co_await acceptor_.async_accept(sock, asio::redirect_error(asio::use_awaitable, ec));
    if (!ec) {
      auto stream = asio::ssl::stream<tcp::socket>(std::move(sock), ssl_ctx_);
      std::make_shared<ssl_session>(std::move(stream), global_chain_)->start();
    }
  }
}

}  // namespace foxhttp
