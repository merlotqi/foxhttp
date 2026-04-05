#include <gtest/gtest.h>

#include <atomic>
#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>
#include <chrono>
#include <foxhttp/foxhttp.hpp>

namespace http = boost::beast::http;

TEST(MiddlewareTimeout, GlobalTimeoutSetting) {
  boost::asio::io_context ioc;
  foxhttp::middleware::MiddlewareChain chain(ioc);

  // Set global timeout
  chain.set_global_timeout(std::chrono::milliseconds(100));

  // No exception should be thrown
  SUCCEED();
}

TEST(MiddlewareTimeout, TimeoutHandlerCanBeSet) {
  boost::asio::io_context ioc;
  foxhttp::middleware::MiddlewareChain chain(ioc);

  std::atomic<bool> handler_called{false};

  chain.set_timeout_handler([&](foxhttp::server::RequestContext& ctx, http::response<http::string_body>& res) {
    handler_called = true;
    res.result(http::status::gateway_timeout);
  });

  // No exception should be thrown
  SUCCEED();
}

TEST(MiddlewareTimeout, MiddlewareWithZeroTimeout) {
  boost::asio::io_context ioc;
  foxhttp::middleware::MiddlewareChain chain(ioc);

  auto mw = std::make_shared<foxhttp::middleware::FunctionalMiddleware>(
      "test",
      [](foxhttp::server::RequestContext&, http::response<http::string_body>&, std::function<void()> next) { next(); });

  chain.use(mw);

  http::request<http::string_body> req;
  req.target("/");
  foxhttp::server::RequestContext ctx(req);
  http::response<http::string_body> res;

  // Execute should complete without timeout
  chain.execute(ctx, res);

  EXPECT_EQ(res.result(), http::status::ok);
}

TEST(MiddlewareTimeout, AsyncExecutionWithTimeout) {
  boost::asio::io_context ioc;
  foxhttp::middleware::MiddlewareChain chain(ioc);

  chain.set_global_timeout(std::chrono::milliseconds(100));

  std::atomic<bool> callback_called{false};

  auto mw = std::make_shared<foxhttp::middleware::FunctionalMiddleware>(
      "test",
      [](foxhttp::server::RequestContext&, http::response<http::string_body>&, std::function<void()> next) { next(); });

  chain.use(mw);

  http::request<http::string_body> req;
  req.target("/");
  foxhttp::server::RequestContext ctx(req);
  http::response<http::string_body> res;

  chain.execute_async(ctx, res, [&](foxhttp::middleware::MiddlewareResult result, const std::string&) {
    callback_called = true;
    EXPECT_EQ(result, foxhttp::middleware::MiddlewareResult::Continue);
  });

  // Run the io_context to process the async operation
  ioc.run();

  EXPECT_TRUE(callback_called);
}

TEST(MiddlewareTimeout, CancelStopsAllTimers) {
  boost::asio::io_context ioc;
  foxhttp::middleware::MiddlewareChain chain(ioc);

  chain.set_global_timeout(std::chrono::milliseconds(1000));

  auto mw = std::make_shared<foxhttp::middleware::FunctionalMiddleware>(
      "test",
      [](foxhttp::server::RequestContext&, http::response<http::string_body>&, std::function<void()> next) { next(); });

  chain.use(mw);

  http::request<http::string_body> req;
  req.target("/");
  foxhttp::server::RequestContext ctx(req);
  http::response<http::string_body> res;

  chain.execute_async(ctx, res, [](foxhttp::middleware::MiddlewareResult, const std::string&) {});

  // Cancel should stop all timers without throwing
  EXPECT_NO_THROW(chain.cancel());

  ioc.run();
}

TEST(MiddlewareTimeout, MultipleConcurrentExecutions) {
  boost::asio::io_context ioc;
  foxhttp::middleware::MiddlewareChain chain(ioc);

  chain.set_global_timeout(std::chrono::milliseconds(100));

  auto mw = std::make_shared<foxhttp::middleware::FunctionalMiddleware>(
      "test",
      [](foxhttp::server::RequestContext&, http::response<http::string_body>&, std::function<void()> next) { next(); });

  chain.use(mw);

  std::atomic<int> callback_count{0};

  // Start multiple concurrent executions
  for (int i = 0; i < 5; ++i) {
    http::request<http::string_body> req;
    req.target("/");
    foxhttp::server::RequestContext ctx(req);
    http::response<http::string_body> res;

    chain.execute_async(ctx, res, [&](foxhttp::middleware::MiddlewareResult, const std::string&) { callback_count++; });
  }

  ioc.run();

  EXPECT_EQ(callback_count, 5);
}
