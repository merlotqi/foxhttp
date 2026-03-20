#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/io_context_pool.hpp>
#include <foxhttp/server/server.hpp>
#include <foxhttp/server/session.hpp>

namespace foxhttp {

server::server(io_context_pool &io_pool, unsigned short port)
    : io_pool_(io_pool),
      listen_io_(&io_pool_.get_io_context()),
      acceptor_(*listen_io_, {tcp::v4(), port}),
      global_chain_(std::make_shared<middleware_chain>(*listen_io_)) {
  do_accept();
}

void server::use(std::shared_ptr<middleware> mw) { global_chain_->use(std::move(mw)); }

std::shared_ptr<middleware_chain> server::global_chain() const { return global_chain_; }

void server::do_accept() {
  acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
    if (!ec) {
      std::make_shared<session>(std::move(socket), global_chain_)->start();
    }
    do_accept();
  });
}

}  // namespace foxhttp
