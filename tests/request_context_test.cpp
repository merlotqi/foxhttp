#include <gtest/gtest.h>

#include <boost/beast/http.hpp>
#include <foxhttp/foxhttp.hpp>
#include <unordered_map>

namespace http = boost::beast::http;

TEST(RequestContext, PathStripsQuery) {
  http::request<http::string_body> req;
  req.target("/foo/bar?x=1");
  foxhttp::server::RequestContext ctx(req);
  EXPECT_EQ(ctx.path(), "/foo/bar");
  EXPECT_EQ(ctx.query(), "x=1");
}

TEST(RequestContext, QueryParametersMultiValue) {
  http::request<http::string_body> req;
  req.target("/p?a=1&b=2&b=3");
  foxhttp::server::RequestContext ctx(req);
  const auto &qp = ctx.query_parameters();
  ASSERT_EQ(qp.count("a"), 1u);
  EXPECT_EQ(qp.at("a").front(), "1");
  ASSERT_EQ(qp.count("b"), 1u);
  ASSERT_EQ(qp.at("b").size(), 2u);
  EXPECT_EQ(qp.at("b")[0], "2");
  EXPECT_EQ(qp.at("b")[1], "3");
}

TEST(RequestContext, PathParametersRoundTrip) {
  http::request<http::string_body> req;
  req.target("/");
  foxhttp::server::RequestContext ctx(req);
  std::unordered_map<std::string, std::string> params{{"id", "7"}};
  ctx.set_path_parameters(params);
  EXPECT_TRUE(ctx.contains_path_parameter("id"));
  EXPECT_EQ(ctx.path_parameter("id"), "7");
}

TEST(RequestContext, ContextBagGetSet) {
  http::request<http::string_body> req;
  req.target("/");
  foxhttp::server::RequestContext ctx(req);
  ctx.set<std::string>("k", "v");
  EXPECT_TRUE(ctx.has("k"));
  EXPECT_EQ(ctx.get<std::string>("k", ""), "v");
}
