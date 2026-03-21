#include <boost/beast/http.hpp>
#include <foxhttp/router/router.hpp>
#include <foxhttp/router/route_table.hpp>
#include <foxhttp/server/request_context.hpp>
#include <gtest/gtest.h>

namespace http = boost::beast::http;

class RouterTest : public ::testing::Test {
 protected:
  void TearDown() override { foxhttp::route_table::instance().clear(); }
};

TEST_F(RouterTest, StaticHandlerResolvesAndRuns) {
  bool hit = false;
  foxhttp::router::register_static_handler(
      "/ping", [&](foxhttp::request_context &ctx, http::response<http::string_body> &res) {
        (void)ctx;
        (void)res;
        hit = true;
      });

  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target("/ping");
  foxhttp::request_context ctx(req);
  auto h = foxhttp::router::resolve_route("/ping", ctx);
  ASSERT_NE(h, nullptr);
  http::response<http::string_body> res;
  h->handle_request(ctx, res);
  EXPECT_TRUE(hit);
}

TEST_F(RouterTest, DynamicHandlerSetsPathParameters) {
  foxhttp::router::register_dynamic_handler(
      "/users/{id}", [&](foxhttp::request_context &ctx, http::response<http::string_body> &res) {
        (void)res;
        EXPECT_EQ(ctx.path_parameter("id"), "42");
      });

  http::request<http::string_body> req;
  req.target("/users/42");
  foxhttp::request_context ctx(req);
  auto h = foxhttp::router::resolve_route("/users/42", ctx);
  ASSERT_NE(h, nullptr);
  http::response<http::string_body> res;
  h->handle_request(ctx, res);
}

TEST_F(RouterTest, UnknownRouteReturnsNull) {
  http::request<http::string_body> req;
  req.target("/nope");
  foxhttp::request_context ctx(req);
  auto h = foxhttp::router::resolve_route("/nope", ctx);
  EXPECT_EQ(h, nullptr);
}

TEST_F(RouterTest, InvalidDynamicPatternThrows) {
  EXPECT_THROW(foxhttp::router::register_dynamic_handler("/bad/{unclosed",
                                                         [](foxhttp::request_context &, http::response<http::string_body> &) {}),
               std::runtime_error);
}

TEST_F(RouterTest, StaticPreferredOverDynamicWhenExact) {
  foxhttp::router::register_static_handler(
      "/api/v1", [](foxhttp::request_context &, http::response<http::string_body> &) {});
  foxhttp::router::register_dynamic_handler(
      "/api/{seg}", [](foxhttp::request_context &, http::response<http::string_body> &) { FAIL() << "dynamic matched"; });

  http::request<http::string_body> req;
  req.target("/api/v1");
  foxhttp::request_context ctx(req);
  auto h = foxhttp::router::resolve_route("/api/v1", ctx);
  ASSERT_NE(h, nullptr);
}

TEST_F(RouterTest, MoreSpecificDynamicRouteWins) {
  std::string captured;
  foxhttp::router::register_dynamic_handler(
      "/api/{seg}/{rest}", [&](foxhttp::request_context &, http::response<http::string_body> &) {
        captured = "two_param";
      });
  foxhttp::router::register_dynamic_handler(
      "/api/v1/{x}", [&](foxhttp::request_context &ctx, http::response<http::string_body> &) {
        captured = "v1:" + ctx.path_parameter("x");
      });

  http::request<http::string_body> req;
  req.target("/api/v1/foo");
  foxhttp::request_context ctx(req);
  auto h = foxhttp::router::resolve_route("/api/v1/foo", ctx);
  ASSERT_NE(h, nullptr);
  http::response<http::string_body> res;
  h->handle_request(ctx, res);
  EXPECT_EQ(captured, "v1:foo");
}
