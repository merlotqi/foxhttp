#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/middleware/basic/functional_middleware.hpp>
#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/request_context.hpp>

namespace http = boost::beast::http;

TEST(MiddlewareBuilder, BuildRunsSyncHandler) {
  int state = 0;
  auto mw = foxhttp::middleware_builder()
                .set_name("built")
                .set_priority(foxhttp::middleware_priority::high)
                .set_sync_func(
                    [&](foxhttp::request_context &, http::response<http::string_body> &, std::function<void()> next) {
                      state = 7;
                      next();
                    })
                .build();

  EXPECT_EQ(mw->name(), "built");
  EXPECT_EQ(mw->priority(), foxhttp::middleware_priority::high);

  http::request<http::string_body> req;
  req.target("/");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;
  mw->operator()(ctx, res, [] {});
  EXPECT_EQ(state, 7);
}

TEST(MiddlewareBuilder, ConditionSkipsWhenUsedInChain) {
  auto outer = foxhttp::middleware_builder()
                   .set_name("outer")
                   .set_sync_func([&](foxhttp::request_context &, http::response<http::string_body> &,
                                      std::function<void()>) { FAIL() << "outer should not run"; })
                   .set_condition([](foxhttp::request_context &) { return false; })
                   .build();

  bool inner_ran = false;
  boost::asio::io_context ioc;
  foxhttp::middleware_chain chain(ioc);
  chain.use(outer);
  chain.use(std::make_shared<foxhttp::functional_middleware>(
      "inner", [&](foxhttp::request_context &, http::response<http::string_body> &, std::function<void()> next) {
        inner_ran = true;
        next();
      }));

  http::request<http::string_body> req;
  req.target("/");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;
  chain.execute(ctx, res);
  EXPECT_TRUE(inner_ran);
}
