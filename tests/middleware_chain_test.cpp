#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/foxhttp.hpp>

namespace http = boost::beast::http;

TEST(MiddlewareChain, EmptyExecuteReturnsImmediately) {
  foxhttp::middleware::MiddlewareChain chain;
  http::request<http::string_body> req;
  req.target("/");
  foxhttp::server::RequestContext ctx(req);
  http::response<http::string_body> res;
  EXPECT_NO_THROW(chain.execute(ctx, res));
}

TEST(MiddlewareChain, SingleMiddlewareRunsAndCallsNext) {
  boost::asio::io_context ioc;
  foxhttp::middleware::MiddlewareChain chain(ioc);

  int order = 0;
  chain.use(std::make_shared<foxhttp::middleware::FunctionalMiddleware>(
      "a", [&](foxhttp::server::RequestContext &, http::response<http::string_body> &, std::function<void()> next) {
        EXPECT_EQ(order, 0);
        order = 1;
        next();
      }));

  http::request<http::string_body> req;
  req.target("/");
  foxhttp::server::RequestContext ctx(req);
  http::response<http::string_body> res;
  chain.execute(ctx, res);
  EXPECT_EQ(order, 1);
}

TEST(MiddlewareChain, TwoMiddlewaresRunInOrder) {
  boost::asio::io_context ioc;
  foxhttp::middleware::MiddlewareChain chain(ioc);
  std::string trace;

  chain.use(std::make_shared<foxhttp::middleware::FunctionalMiddleware>(
      "first", [&](foxhttp::server::RequestContext &, http::response<http::string_body> &, std::function<void()> next) {
        trace += "1";
        next();
      }));
  chain.use(std::make_shared<foxhttp::middleware::FunctionalMiddleware>(
      "second", [&](foxhttp::server::RequestContext &, http::response<http::string_body> &, std::function<void()> next) {
        trace += "2";
        next();
      }));

  http::request<http::string_body> req;
  req.target("/");
  foxhttp::server::RequestContext ctx(req);
  http::response<http::string_body> res;
  chain.execute(ctx, res);
  EXPECT_EQ(trace, "12");
}
