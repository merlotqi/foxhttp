#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/request_context.hpp>
#include <memory>

namespace foxhttp::detail {

/// Suspend the current coroutine until `middleware_chain::execute_async` invokes its completion handler.
/// Completion is always posted via `asio::post` so synchronous `execute_async` callbacks still resume safely.
inline boost::asio::awaitable<void> await_middleware_chain_async(
    boost::asio::any_io_executor ex, std::shared_ptr<middleware_chain> chain, request_context &ctx,
    boost::beast::http::response<boost::beast::http::string_body> &res) {
  namespace asio = boost::asio;

  asio::steady_timer done{ex};
  done.expires_at(asio::steady_timer::time_point::max());

  chain->execute_async(ctx, res, [ex, raw = &done](middleware_result, const std::string &) {
    asio::post(ex, [raw]() {
      boost::system::error_code ignored;
      raw->cancel(ignored);
    });
  });

  boost::system::error_code ec;
  co_await done.async_wait(asio::redirect_error(asio::use_awaitable, ec));
}

}  // namespace foxhttp::detail
