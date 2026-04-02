// Example: BodyParserMiddleware + JSON POST echo using RequestContext::parsed_body.
//
// Run: ./build/examples/05_json_body
// Try: curl -s -X POST http://127.0.0.1:8084/api/echo -H "Content-Type: application/json" -d '{"a":1}'

#include <foxhttp/foxhttp.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

#include "detail/router_dispatch_middleware.hpp"

namespace http = boost::beast::http;

int main() {
  const unsigned short port = 8084;

  try {
    foxhttp::server::IoContextPool pool(1);
    auto sigs = std::make_shared<foxhttp::server::SignalSet>(pool.get_io_context());
    sigs->register_handler(SIGINT, [&pool](int, const std::string &) { pool.stop(); });
    sigs->start();

    foxhttp::server::Server server(pool, port);

    server.use(std::make_shared<foxhttp::middleware::BodyParserMiddleware>());
    server.use(std::make_shared<foxhttp::examples::router_dispatch_middleware>());

    foxhttp::router::Router::register_static_handler(
        "/", [](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
          (void)ctx;
          res.result(http::status::ok);
          res.set(http::field::content_type, "text/plain");
          res.body() = "Example 05 — POST JSON to /api/echo with Content-Type: application/json\n";
        });

    foxhttp::router::Router::register_static_handler(
        "/api/echo", [](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
          if (ctx.method() != http::verb::post) {
            res.result(http::status::method_not_allowed);
            res.set(http::field::content_type, "text/plain");
            res.body() = "Use POST with a JSON body\n";
            return;
          }
          try {
            const auto &j = ctx.parsed_body<nlohmann::json>();
            res.result(http::status::ok);
            res.set(http::field::content_type, "application/json");
            res.body() = j.dump();
          } catch (const std::exception &) {
            res.result(http::status::bad_request);
            res.set(http::field::content_type, "text/plain");
            res.body() = "Expected JSON body (middleware may have skipped parsing)\n";
          }
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
