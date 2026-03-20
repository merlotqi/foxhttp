/**
 * foxhttp - static + dynamic route registration with lambdas (same handler signature)
 */

#include <foxhttp/foxhttp.hpp>
#include <iostream>

namespace http = boost::beast::http;

static void run_request(const char *target) {
  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target(target);
  req.version(11);
  req.set(http::field::host, "localhost");
  req.body() = "";
  req.prepare_payload();

  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;
  res.version(req.version());
  res.keep_alive(false);

  auto h = foxhttp::router::resolve_route(ctx.path(), ctx);
  if (h) {
    h->handle_request(ctx, res);
  } else {
    res.result(http::status::not_found);
    res.set(http::field::content_type, "text/plain");
    res.body() = "not found";
  }
  res.prepare_payload();

  std::cout << target << " -> " << static_cast<unsigned>(res.result_int()) << " " << res.body() << std::endl;
}

int main() {
  foxhttp::router::register_static_handler(
      "/api/version", [](foxhttp::request_context &ctx, http::response<http::string_body> &res) {
        (void)ctx;
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = R"({"version":"0.1.0"})";
      });

  foxhttp::router::register_dynamic_handler(
      "/api/users/{id}", [](foxhttp::request_context &ctx, http::response<http::string_body> &res) {
        res.result(http::status::ok);
        res.set(http::field::content_type, "text/plain");
        res.body() = "user:" + ctx.path_parameter("id");
      });

  run_request("/api/version");
  run_request("/api/users/42");
  run_request("/api/missing");

  return 0;
}
