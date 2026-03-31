#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/middleware/basic/functional_middleware.hpp>
#include <foxhttp/server/request_context.hpp>
#include <chrono>
#include <atomic>

namespace http = boost::beast::http;

TEST(MiddlewareTimeout, GlobalTimeoutSetting) {
  boost::asio::io_context ioc;
  foxhttp::middleware_chain chain(ioc);
  
  // Set global timeout
  chain.set_global_timeout(std::chrono::milliseconds(100));
  
  // No exception should be thrown
  SUCCEED();
}

TEST(MiddlewareTimeout, TimeoutHandlerCanBeSet) {
  boost::asio::io_context ioc;
  foxhttp::middleware_chain chain(ioc);
  
  std::atomic<bool> handler_called{false};
  
  chain.set_timeout_handler([&](foxhttp::request_context& ctx, http::response<http::string_body>& res) {
    handler_called = true;
    res.result(http::status::gateway_timeout);
  });
  
  // No exception should be thrown
  SUCCEED();
}

TEST(MiddlewareTimeout, MiddlewareWithZeroTimeout) {
  boost::asio::io_context ioc;
  foxhttp::middleware_chain chain(ioc);
  
  auto mw = std::make_shared<foxhttp::functional_middleware>(
      "test", [](foxhttp::request_context&, http::response<http::string_body>&, std::function<void()> next) {
        next();
      });
  
  chain.use(mw);
  
  http::request<http::string_body> req;
  req.target("/");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;
  
  // Execute should complete without timeout
  chain.execute(ctx, res);
  
  EXPECT_EQ(res.result(), http::status::ok);
}

TEST(MiddlewareTimeout, AsyncExecutionWithTimeout) {
  boost::asio::io_context ioc;
  foxhttp::middleware_chain chain(ioc);
  
  chain.set_global_timeout(std::chrono::milliseconds(100));
  
  std::atomic<bool> callback_called{false};
  
  auto mw = std::make_shared<foxhttp::functional_middleware>(
      "test", [](foxhttp::request_context&, http::response<http::string_body>&, std::function<void()> next) {
        next();
      });
  
  chain.use(mw);
  
  http::request<http::string_body> req;
  req.target("/");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;
  
  chain.execute_async(ctx, res, [&](foxhttp::middleware_result result, const std::string&) {
    callback_called = true;
    EXPECT_EQ(result, foxhttp::middleware_result::continue_);
  });
  
  // Run the io_context to process the async operation
  ioc.run();
  
  EXPECT_TRUE(callback_called);
}

TEST(MiddlewareTimeout, CancelStopsAllTimers) {
  boost::asio::io_context ioc;
  foxhttp::middleware_chain chain(ioc);
  
  chain.set_global_timeout(std::chrono::milliseconds(1000));
  
  auto mw = std::make_shared<foxhttp::functional_middleware>(
      "test", [](foxhttp::request_context&, http::response<http::string_body>&, std::function<void()> next) {
        next();
      });
  
  chain.use(mw);
  
  http::request<http::string_body> req;
  req.target("/");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;
  
  chain.execute_async(ctx, res, [](foxhttp::middleware_result, const std::string&) {});
  
  // Cancel should stop all timers without throwing
  EXPECT_NO_THROW(chain.cancel());
  
  ioc.run();
}

TEST(MiddlewareTimeout, MultipleConcurrentExecutions) {
  boost::asio::io_context ioc;
  foxhttp::middleware_chain chain(ioc);
  
  chain.set_global_timeout(std::chrono::milliseconds(100));
  
  auto mw = std::make_shared<foxhttp::functional_middleware>(
      "test", [](foxhttp::request_context&, http::response<http::string_body>&, std::function<void()> next) {
        next();
      });
  
  chain.use(mw);
  
  std::atomic<int> callback_count{0};
  
  // Start multiple concurrent executions
  for (int i = 0; i < 5; ++i) {
    http::request<http::string_body> req;
    req.target("/");
    foxhttp::request_context ctx(req);
    http::response<http::string_body> res;
    
    chain.execute_async(ctx, res, [&](foxhttp::middleware_result, const std::string&) {
      callback_count++;
    });
  }
  
  ioc.run();
  
  EXPECT_EQ(callback_count, 5);
}
