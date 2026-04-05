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

asio::awaitable<void> run_await_chain(asio::io_context &ioc, std::shared_ptr<foxhttp::middleware_chain> chain,
                                      foxhttp::request_context &ctx, http::response<http::string_body> &res) {
  co_await foxhttp::detail::await_middleware_chain_async(ioc.get_executor(), chain, ctx, res);
}

}  // namespace

TEST(AwaitMiddlewareAsync, SyncMiddlewareCompletes) {
  asio::io_context ioc;
  auto chain = std::make_shared<foxhttp::middleware_chain>(ioc);
  chain->use(std::make_shared<foxhttp::functional_middleware>(
      "sync", [](foxhttp::request_context &, http::response<http::string_body> &res, std::function<void()> next) {
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
  foxhttp::request_context ctx(req);

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
  auto chain = std::make_shared<foxhttp::middleware_chain>(ioc);
  chain->use(std::make_shared<foxhttp::functional_middleware>(
      "async",
      [](foxhttp::request_context &, http::response<http::string_body> &, std::function<void()> next) { next(); },
      [&ioc](foxhttp::request_context &, http::response<http::string_body> &res, std::function<void()>,
             foxhttp::async_middleware_callback cb) {
        asio::post(ioc.get_executor(), [&res, cb]() {
          res.result(http::status::accepted);
          res.body() = "async";
          cb(foxhttp::middleware_result::continue_, "");
        });
      }));

  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target("/");
  req.version(11);

  http::response<http::string_body> res{http::status::internal_server_error, 11};
  foxhttp::request_context ctx(req);

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
  auto chain = std::make_shared<foxhttp::middleware_chain>(ioc);

  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target("/");
  req.version(11);

  http::response<http::string_body> res{http::status::ok, 11};
  foxhttp::request_context ctx(req);

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
