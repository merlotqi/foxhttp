#include <gtest/gtest.h>

#include <boost/beast/http.hpp>
#include <foxhttp/middleware/basic/functional_middleware.hpp>
#include <foxhttp/middleware/basic/response_time_middleware.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/request_context.hpp>

namespace http = boost::beast::http;

TEST(ResponseTimeMiddleware, AddsHeaderAfterInnerMiddleware) {
  boost::asio::io_context ioc;
  foxhttp::middleware_chain chain(ioc);

  chain.use(std::make_shared<foxhttp::response_time_middleware>("X-RT"));
  chain.use(std::make_shared<foxhttp::functional_middleware>(
      "noop",
      [](foxhttp::request_context &, http::response<http::string_body> &, std::function<void()> next) { next(); }));

  http::request<http::string_body> req;
  req.target("/");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;
  chain.execute(ctx, res);

  ASSERT_TRUE(res.find("X-RT") != res.end());
  std::string v(res["X-RT"]);
  EXPECT_NE(v.find("ms"), std::string::npos);
}
