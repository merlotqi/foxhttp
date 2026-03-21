#include <boost/beast/http.hpp>
#include <foxhttp/middleware/basic/body_parser_middleware.hpp>
#include <foxhttp/parser/form_parser.hpp>
#include <foxhttp/parser/json_parser.hpp>
#include <foxhttp/server/request_context.hpp>
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

namespace http = boost::beast::http;

TEST(BodyParserMiddleware, JsonBodyStoredInParsedBody) {
  http::request<http::string_body> req;
  req.method(http::verb::post);
  req.set(http::field::content_type, "application/json; charset=utf-8");
  req.body() = R"({"n":42})";

  foxhttp::request_context ctx(req);
  foxhttp::body_parser_middleware mw;
  http::response<http::string_body> res;
  bool next_called = false;
  mw(ctx, res, [&] { next_called = true; });

  EXPECT_TRUE(next_called);
  EXPECT_EQ(res.result(), http::status::ok);
  EXPECT_EQ(ctx.parsed_body<nlohmann::json>()["n"], 42);
}

TEST(BodyParserMiddleware, EmptyBodySkipsParsing) {
  http::request<http::string_body> req;
  req.set(http::field::content_type, "application/json");
  req.body() = "";

  foxhttp::request_context ctx(req);
  foxhttp::body_parser_middleware mw;
  http::response<http::string_body> res;
  bool next_called = false;
  mw(ctx, res, [&] { next_called = true; });
  EXPECT_TRUE(next_called);
}

TEST(BodyParserMiddleware, NoContentTypeSkipsParsing) {
  http::request<http::string_body> req;
  req.body() = "{}";

  foxhttp::request_context ctx(req);
  foxhttp::body_parser_middleware mw;
  http::response<http::string_body> res;
  bool next_called = false;
  mw(ctx, res, [&] { next_called = true; });
  EXPECT_TRUE(next_called);
}

TEST(BodyParserMiddleware, InvalidJsonReturns400WhenEnabled) {
  http::request<http::string_body> req;
  req.set(http::field::content_type, "application/json");
  req.body() = "not-json";

  foxhttp::request_context ctx(req);
  foxhttp::body_parser_middleware mw("BodyParser", true);
  http::response<http::string_body> res;
  mw(ctx, res, [] { FAIL() << "next should not run"; });
  EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST(BodyParserMiddleware, FormUrlEncodedStoresUnderContextKey) {
  http::request<http::string_body> req;
  req.method(http::verb::post);
  req.set(http::field::content_type, "application/x-www-form-urlencoded");
  req.body() = "a=1&b=two";

  foxhttp::request_context ctx(req);
  foxhttp::body_parser_middleware mw;
  http::response<http::string_body> res;
  mw(ctx, res, [] {});

  auto fd = ctx.get<std::shared_ptr<foxhttp::form_data>>(foxhttp::body_parser_context_keys::form, nullptr);
  ASSERT_NE(fd, nullptr);
  ASSERT_NE(fd->find("a"), fd->end());
  ASSERT_NE(fd->find("b"), fd->end());
  EXPECT_EQ(fd->at("a")->value(), "1");
  EXPECT_EQ(fd->at("b")->value(), "two");
}

TEST(BodyParserMiddleware, TextPlainStoredAsString) {
  http::request<http::string_body> req;
  req.set(http::field::content_type, "text/plain; charset=utf-8");
  req.body() = "plain";

  foxhttp::request_context ctx(req);
  foxhttp::body_parser_middleware mw;
  http::response<http::string_body> res;
  mw(ctx, res, [] {});

  EXPECT_EQ(ctx.parsed_body<std::string>(), "plain");
}
