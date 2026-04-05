// Example: minimal HTTP server — IoContextPool, server, router, terminal dispatch middleware.
//
// Build: cmake -S . -B build -DFOXHTTP_BUILD_EXAMPLES=ON && cmake --build build
// Run:   ./build/examples/01_hello_server
// Try:   curl -s http://127.0.0.1:8080/health

#include <csignal>
#include <foxhttp/foxhttp.hpp>
#include <iostream>
#include <thread>

#include "detail/router_dispatch_middleware.hpp"

namespace http = boost::beast::http;

int main() {
  const unsigned short port = 8080;

  try {
    foxhttp::server::IoContextPool pool(1);

    auto sigs = std::make_shared<foxhttp::server::SignalSet>(pool.get_io_context());
    sigs->register_handler(SIGINT, [&pool](int, const std::string &) {
      std::cerr << "\nStopping...\n";
      pool.stop();
    });
#ifndef _WIN32
    sigs->register_handler(SIGTERM, [&pool](int, const std::string &) { pool.stop(); });
#endif
    sigs->start();

    foxhttp::server::Server server(pool, port);
    server.use(std::make_shared<foxhttp::examples::router_dispatch_middleware>());

    foxhttp::router::Router::register_static_handler("/",
                                             [](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
                                               (void)ctx;
                                               res.result(http::status::ok);
                                               res.set(http::field::content_type, "text/plain");
                                               res.body() =
                                                   "FoxHttp example 01 — hello server.\n"
                                                   "Try: GET /health  GET /api/version\n";
                                             });

    foxhttp::router::Router::register_static_handler("/health",
                                             [](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
                                               (void)ctx;
                                               res.result(http::status::ok);
                                               res.set(http::field::content_type, "application/json");
                                               res.body() = R"({"status":"ok"})";
                                             });

    foxhttp::router::Router::register_static_handler("/api/version",
                                             [](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
                                               (void)ctx;
                                               res.result(http::status::ok);
                                               res.set(http::field::content_type, "application/json");
                                               res.body() = R"({"example":"01_hello_server"})";
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
