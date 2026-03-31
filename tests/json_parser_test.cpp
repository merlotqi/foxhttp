#include <gtest/gtest.h>

#include <boost/beast/http.hpp>
#include <foxhttp/foxhttp.hpp>

namespace http = boost::beast::http;

TEST(JsonParser, SupportsApplicationJson) {
  foxhttp::json_parser p;
  http::request<http::string_body> req;
  req.set(http::field::content_type, "application/json");
  req.body() = "{}";
  EXPECT_TRUE(p.supports(req));

  http::request<http::string_body> req2;
  req2.set(http::field::content_type, "application/json; charset=utf-8");
  req2.body() = "{}";
  EXPECT_TRUE(p.supports(req2));
}

TEST(JsonParser, ParseObject) {
  foxhttp::json_parser p;
  http::request<http::string_body> req;
  req.set(http::field::content_type, "application/json");
  req.body() = R"({"ok":true,"n":3})";

  auto j = p.parse(req);
  EXPECT_TRUE(j["ok"].get<bool>());
  EXPECT_EQ(j["n"].get<int>(), 3);
}

TEST(JsonParser, RejectsNonJsonContentType) {
  foxhttp::json_parser p;
  http::request<http::string_body> req;
  req.set(http::field::content_type, "text/plain");
  req.body() = "{}";
  EXPECT_FALSE(p.supports(req));
}
