#include <gtest/gtest.h>

#include <boost/beast/http.hpp>
#include <foxhttp/foxhttp.hpp>

namespace http = boost::beast::http;

TEST(CorsMiddleware, OptionsPreflightDoesNotCallNext) {
  foxhttp::cors_middleware mw("https://example.com", "GET, POST", "Content-Type", false, 3600);

  http::request<http::string_body> req;
  req.method(http::verb::options);
  req.target("/");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;

  bool next_called = false;
  mw(ctx, res, [&] { next_called = true; });

  EXPECT_FALSE(next_called);
  EXPECT_EQ(res.result(), http::status::ok);
  EXPECT_EQ(res["Access-Control-Allow-Origin"], "https://example.com");
  EXPECT_EQ(res["Access-Control-Allow-Methods"], "GET, POST");
}

TEST(CorsMiddleware, GetPassesThroughAndSetsHeaders) {
  foxhttp::cors_middleware mw("*", "GET", "X-Custom", false, 60);

  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target("/");
  req.set("Origin", "https://app.example");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;

  bool next_called = false;
  mw(ctx, res, [&] { next_called = true; });

  EXPECT_TRUE(next_called);
  EXPECT_EQ(res["Access-Control-Allow-Origin"], "https://app.example");
  EXPECT_EQ(res["Access-Control-Allow-Methods"], "GET");
}
