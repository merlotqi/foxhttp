// Example: middleware ordering — CORS, simple logger, response-time header, then router dispatch.
//
// Run: ./build/examples/03_middleware_chain
// Try: curl -i http://127.0.0.1:8082/api/hello

#include <foxhttp/foxhttp.hpp>
#include <iostream>
#include <thread>

#include "detail/first_cors_middleware.hpp"
#include "detail/router_dispatch_middleware.hpp"

namespace http = boost::beast::http;

int main() {
  const unsigned short port = 8082;

  try {
    foxhttp::server::IoContextPool pool(1);
    auto sigs = std::make_shared<foxhttp::server::SignalSet>(pool.get_io_context());
    sigs->register_handler(SIGINT, [&pool](int, const std::string &) { pool.stop(); });
    sigs->start();

    foxhttp::server::Server server(pool, port);

    // MiddlewareChain sorts by priority (lower value runs first). Use first_cors_middleware so CORS
    // runs before logger (high) and response_time (low); router_dispatch is "highest" and runs last.
    server.use(std::make_shared<foxhttp::examples::first_cors_middleware>("*", "GET, POST, OPTIONS", "Content-Type"));
    server.use(foxhttp::middleware::factories::create_simple_logger("example03"));
    server.use(std::make_shared<foxhttp::middleware::ResponseTimeMiddleware>("X-Response-Time"));
    server.use(std::make_shared<foxhttp::examples::router_dispatch_middleware>());

    foxhttp::router::Router::register_static_handler(
        "/", [](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
          (void)ctx;
          res.result(http::status::ok);
          res.set(http::field::content_type, "text/plain");
          res.body() = "Example 03 — see X-Response-Time and server logs.\nGET /api/hello\n";
        });

    foxhttp::router::Router::register_static_handler("/api/hello",
                                             [](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
                                               (void)ctx;
                                               res.result(http::status::ok);
                                               res.set(http::field::content_type, "application/json");
                                               res.body() = R"({"message":"hello"})";
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
