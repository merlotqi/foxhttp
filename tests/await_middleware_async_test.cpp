#include <gtest/gtest.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/foxhttp.hpp>

namespace http = boost::beast::http;
namespace asio = boost::asio;

namespace {

asio::awaitable<void> run_await_chain(asio::io_context &ioc, std::shared_ptr<foxhttp::middleware::MiddlewareChain> chain,
                                      foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
  co_await foxhttp::detail::await_middleware_chain_async(ioc.get_executor(), chain, ctx, res);
}

}  // namespace

TEST(AwaitMiddlewareAsync, SyncMiddlewareCompletes) {
  asio::io_context ioc;
  auto chain = std::make_shared<foxhttp::middleware::MiddlewareChain>(ioc);
  chain->use(std::make_shared<foxhttp::middleware::FunctionalMiddleware>(
      "sync", [](foxhttp::server::RequestContext &, http::response<http::string_body> &res, std::function<void()> next) {
        res.result(http::status::ok);
        res.set(http::field::content_type, "text/plain");
        res.body() = "hello";
        next();
      }));

  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target("/");
  req.version(11);

  http::response<http::string_body> res{http::status::internal_server_error, 11};
  foxhttp::server::RequestContext ctx(req);

  bool finished = false;
  asio::co_spawn(
      ioc,
      [&]() -> asio::awaitable<void> {
        co_await run_await_chain(ioc, chain, ctx, res);
        finished = true;
      },
      asio::detached);

  ioc.run();
  EXPECT_TRUE(finished);
  EXPECT_EQ(res.result(), http::status::ok);
  EXPECT_EQ(res.body(), "hello");
}

TEST(AwaitMiddlewareAsync, AsyncMiddlewareCompletesViaPost) {
  asio::io_context ioc;
  auto chain = std::make_shared<foxhttp::middleware::MiddlewareChain>(ioc);
  chain->use(std::make_shared<foxhttp::middleware::FunctionalMiddleware>(
      "async",
      [](foxhttp::server::RequestContext &, http::response<http::string_body> &, std::function<void()> next) { next(); },
      [&ioc](foxhttp::server::RequestContext &, http::response<http::string_body> &res, std::function<void()>,
             foxhttp::middleware::async_middleware_callback cb) {
        asio::post(ioc.get_executor(), [&res, cb]() {
          res.result(http::status::accepted);
          res.body() = "async";
          cb(foxhttp::middleware::MiddlewareResult::Continue, "");
        });
      }));

  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target("/");
  req.version(11);

  http::response<http::string_body> res{http::status::internal_server_error, 11};
  foxhttp::server::RequestContext ctx(req);

  bool finished = false;
  asio::co_spawn(
      ioc,
      [&]() -> asio::awaitable<void> {
        co_await run_await_chain(ioc, chain, ctx, res);
        finished = true;
      },
      asio::detached);

  ioc.run();
  EXPECT_TRUE(finished);
  EXPECT_EQ(res.result(), http::status::accepted);
  EXPECT_EQ(res.body(), "async");
}

TEST(AwaitMiddlewareAsync, EmptyChainCompletes) {
  asio::io_context ioc;
  auto chain = std::make_shared<foxhttp::middleware::MiddlewareChain>(ioc);

  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target("/");
  req.version(11);

  http::response<http::string_body> res{http::status::ok, 11};
  foxhttp::server::RequestContext ctx(req);

  bool finished = false;
  asio::co_spawn(
      ioc,
      [&]() -> asio::awaitable<void> {
        co_await run_await_chain(ioc, chain, ctx, res);
        finished = true;
      },
      asio::detached);

  ioc.run();
  EXPECT_TRUE(finished);
}
