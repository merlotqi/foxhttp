#include <gtest/gtest.h>

#include <boost/beast/http.hpp>
#include <foxhttp/foxhttp.hpp>

namespace http = boost::beast::http;

TEST(ResponseTimeMiddleware, AddsHeaderAfterInnerMiddleware) {
  boost::asio::io_context ioc;
  foxhttp::middleware::MiddlewareChain chain(ioc);

  chain.use(std::make_shared<foxhttp::middleware::ResponseTimeMiddleware>("X-RT"));
  chain.use(std::make_shared<foxhttp::middleware::FunctionalMiddleware>(
      "noop",
      [](foxhttp::server::RequestContext &, http::response<http::string_body> &, std::function<void()> next) { next(); }));

  http::request<http::string_body> req;
  req.target("/");
  foxhttp::server::RequestContext ctx(req);
  http::response<http::string_body> res;
  chain.execute(ctx, res);

  ASSERT_TRUE(res.find("X-RT") != res.end());
  std::string v(res["X-RT"]);
  EXPECT_NE(v.find("ms"), std::string::npos);
}
