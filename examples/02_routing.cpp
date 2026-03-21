// Example: static and dynamic routes with path parameters.
//
// Run: ./build/examples/02_routing
// Try: curl -s http://127.0.0.1:8081/api/users/42
//      curl -s http://127.0.0.1:8081/api/items/foo/bar

#include <foxhttp/foxhttp.hpp>

#include "detail/router_dispatch_middleware.hpp"

#include <iostream>
#include <sstream>
#include <thread>

namespace http = boost::beast::http;

int main() {
  const unsigned short port = 8081;

  try {
    foxhttp::io_context_pool pool(1);
    auto sigs = std::make_shared<foxhttp::signal_set>(pool.get_io_context());
    sigs->register_handler(SIGINT, [&pool](int, const std::string &) { pool.stop(); });
    sigs->start();

    foxhttp::server server(pool, port);
    server.use(std::make_shared<foxhttp::examples::router_dispatch_middleware>());

    foxhttp::router::register_static_handler(
        "/", [](foxhttp::request_context &ctx, http::response<http::string_body> &res) {
          (void)ctx;
          res.result(http::status::ok);
          res.set(http::field::content_type, "text/plain");
          res.body() = "Example 02 — routing. Try /api/users/{id} and /api/items/{category}/{id}\n";
        });

    foxhttp::router::register_dynamic_handler(
        "/api/users/{id}", [](foxhttp::request_context &ctx, http::response<http::string_body> &res) {
          res.result(http::status::ok);
          res.set(http::field::content_type, "application/json");
          res.body() = std::string(R"({"user_id":")") + ctx.path_parameter("id") + "\"}";
        });

    foxhttp::router::register_dynamic_handler(
        "/api/items/{category}/{id}", [](foxhttp::request_context &ctx, http::response<http::string_body> &res) {
          res.result(http::status::ok);
          res.set(http::field::content_type, "application/json");
          std::ostringstream os;
          os << R"({"category":")" << ctx.path_parameter("category") << R"(","id":")" << ctx.path_parameter("id")
             << "\"}";
          res.body() = os.str();
        });

    std::cerr << "Listening on http://127.0.0.1:" << port << "/ (Ctrl+C to stop)\n";

    std::thread runner([&pool] { pool.run_blocking(); });
    runner.join();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
