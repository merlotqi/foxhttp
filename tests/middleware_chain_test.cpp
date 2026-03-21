#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/middleware/basic/functional_middleware.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/request_context.hpp>
#include <gtest/gtest.h>

namespace http = boost::beast::http;

TEST(MiddlewareChain, EmptyExecuteReturnsImmediately) {
  foxhttp::middleware_chain chain;
  http::request<http::string_body> req;
  req.target("/");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;
  EXPECT_NO_THROW(chain.execute(ctx, res));
}

TEST(MiddlewareChain, SingleMiddlewareRunsAndCallsNext) {
  boost::asio::io_context ioc;
  foxhttp::middleware_chain chain(ioc);

  int order = 0;
  chain.use(std::make_shared<foxhttp::functional_middleware>(
      "a", [&](foxhttp::request_context &, http::response<http::string_body> &, std::function<void()> next) {
        EXPECT_EQ(order, 0);
        order = 1;
        next();
      }));

  http::request<http::string_body> req;
  req.target("/");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;
  chain.execute(ctx, res);
  EXPECT_EQ(order, 1);
}

TEST(MiddlewareChain, TwoMiddlewaresRunInOrder) {
  boost::asio::io_context ioc;
  foxhttp::middleware_chain chain(ioc);
  std::string trace;

  chain.use(std::make_shared<foxhttp::functional_middleware>(
      "first", [&](foxhttp::request_context &, http::response<http::string_body> &, std::function<void()> next) {
        trace += "1";
        next();
      }));
  chain.use(std::make_shared<foxhttp::functional_middleware>(
      "second", [&](foxhttp::request_context &, http::response<http::string_body> &, std::function<void()> next) {
        trace += "2";
        next();
      }));

  http::request<http::string_body> req;
  req.target("/");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;
  chain.execute(ctx, res);
  EXPECT_EQ(trace, "12");
}
