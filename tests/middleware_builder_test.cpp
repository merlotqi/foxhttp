#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/foxhttp.hpp>

namespace http = boost::beast::http;

TEST(MiddlewareBuilder, BuildRunsSyncHandler) {
  int state = 0;
  auto mw = foxhttp::middleware::MiddlewareBuilder()
                .set_name("built")
                .set_priority(foxhttp::middleware::MiddlewarePriority::High)
                .set_sync_func(
                    [&](foxhttp::server::RequestContext &, http::response<http::string_body> &, std::function<void()> next) {
                      state = 7;
                      next();
                    })
                .build();

  EXPECT_EQ(mw->name(), "built");
  EXPECT_EQ(mw->priority(), foxhttp::middleware::MiddlewarePriority::High);

  http::request<http::string_body> req;
  req.target("/");
  foxhttp::server::RequestContext ctx(req);
  http::response<http::string_body> res;
  mw->operator()(ctx, res, [] {});
  EXPECT_EQ(state, 7);
}

TEST(MiddlewareBuilder, ConditionSkipsWhenUsedInChain) {
  auto outer = foxhttp::middleware::MiddlewareBuilder()
                   .set_name("outer")
                   .set_sync_func([&](foxhttp::server::RequestContext &, http::response<http::string_body> &,
                                      std::function<void()>) { FAIL() << "outer should not run"; })
                   .set_condition([](foxhttp::server::RequestContext &) { return false; })
                   .build();

  bool inner_ran = false;
  boost::asio::io_context ioc;
  foxhttp::middleware::MiddlewareChain chain(ioc);
  chain.use(outer);
  chain.use(std::make_shared<foxhttp::middleware::FunctionalMiddleware>(
      "inner", [&](foxhttp::server::RequestContext &, http::response<http::string_body> &, std::function<void()> next) {
        inner_ran = true;
        next();
      }));

  http::request<http::string_body> req;
  req.target("/");
  foxhttp::server::RequestContext ctx(req);
  http::response<http::string_body> res;
  chain.execute(ctx, res);
  EXPECT_TRUE(inner_ran);
}
