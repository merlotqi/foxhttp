#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/config.hpp>
#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/request_context.hpp>
#include <memory>

#if FOXHTTP_HAS_COROUTINES
#include <boost/asio/awaitable.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#endif

namespace foxhttp::detail {

#if FOXHTTP_HAS_COROUTINES

/// Suspend the current coroutine until `MiddlewareChain::execute_async` invokes its completion handler.
/// Completion is always posted via `asio::post` so synchronous `execute_async` callbacks still resume safely.
/// Only available when built with C++20 (FOXHTTP_HAS_COROUTINES = 1).
inline boost::asio::awaitable<void> await_middleware_chain_async(
    boost::asio::any_io_executor ex, std::shared_ptr<middleware::MiddlewareChain> chain,
    server::RequestContext &ctx, boost::beast::http::response<boost::beast::http::string_body> &res) {
  namespace asio = boost::asio;

  asio::steady_timer done{ex};
  done.expires_at(asio::steady_timer::time_point::max());

  chain->execute_async(ctx, res, [ex, raw = &done](middleware::MiddlewareResult, const std::string &) {
    asio::post(ex, [raw]() {
      boost::system::error_code ignored;
      raw->cancel(ignored);
    });
  });

  boost::system::error_code ec;
  co_await done.async_wait(asio::redirect_error(asio::use_awaitable, ec));
}

#endif  // FOXHTTP_HAS_COROUTINES

}  // namespace foxhttp::detail
