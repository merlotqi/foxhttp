#pragma once

#include <atomic>
#include <boost/asio/ip/tcp.hpp>
#include <foxhttp/config.hpp>
#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/router/router.hpp>
#include <memory>
#include <type_traits>

#if FOXHTTP_HAS_COROUTINES
#include <boost/asio/awaitable.hpp>
#endif

using tcp = boost::asio::ip::tcp;

namespace foxhttp::server {

class IoContextPool;

using Middleware = middleware::Middleware;
using MiddlewareChain = middleware::MiddlewareChain;
using Router = router::Router;

class Server {
 public:
  Server(IoContextPool &io_pool, unsigned short port);

  /// Add middleware to the global chain
  void use(std::shared_ptr<Middleware> mw);

  /// Template version for derived middleware types
  template <typename MW, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<MW>, Middleware>>>
  void use(std::shared_ptr<MW> mw) {
    use(std::static_pointer_cast<Middleware>(std::move(mw)));
  }

  std::shared_ptr<MiddlewareChain> global_chain() const;

  /// Bind a router instance to this server (alternative to using router_dispatch_middleware)
  void bind_router(std::shared_ptr<Router> router);

  /// Get the bound router (if any)
  std::shared_ptr<Router> bound_router() const;

  void stop();

 private:
  void do_accept();
#if FOXHTTP_HAS_COROUTINES
  boost::asio::awaitable<void> accept_loop();
#else
  void accept_loop();
#endif

  IoContextPool &io_pool_;
  boost::asio::io_context *listen_io_;
  tcp::acceptor acceptor_;
  std::shared_ptr<MiddlewareChain> global_chain_;
  std::shared_ptr<Router> router_;
  std::atomic<bool> stopping_{false};
};

}  // namespace foxhttp::server
