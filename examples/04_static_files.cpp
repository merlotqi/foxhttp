// Example: StaticMiddleware for files under /static/ plus JSON API routes.
//
// WWW root is set at compile time (FOXHTTP_EXAMPLE_WWW_DIR). Default files live in examples/www/.
//
// Run: ./build/examples/04_static_files
// Try: curl -s http://127.0.0.1:8083/static/index.html
//      curl -s http://127.0.0.1:8083/api/ping

#include <filesystem>
#include <foxhttp/foxhttp.hpp>
#include <iostream>
#include <thread>

#include "detail/router_dispatch_middleware.hpp"

namespace http = boost::beast::http;

#ifndef FOXHTTP_EXAMPLE_WWW_DIR
#define FOXHTTP_EXAMPLE_WWW_DIR "."
#endif

int main() {
  const unsigned short port = 8083;
  const std::filesystem::path www{FOXHTTP_EXAMPLE_WWW_DIR};

  try {
    foxhttp::server::IoContextPool pool(1);
    auto sigs = std::make_shared<foxhttp::server::SignalSet>(pool.get_io_context());
    sigs->register_handler(SIGINT, [&pool](int, const std::string &) { pool.stop(); });
    sigs->start();

    foxhttp::server::Server server(pool, port);

    server.use(std::make_shared<foxhttp::middleware::StaticMiddleware>("/static", www));
    server.use(std::make_shared<foxhttp::examples::router_dispatch_middleware>());

    foxhttp::router::Router::register_static_handler(
        "/", [](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
          (void)ctx;
          res.result(http::status::ok);
          res.set(http::field::content_type, "text/plain");
          res.body() = "Example 04 — static files at /static/ (from examples/www). API: GET /api/ping\n";
        });

    foxhttp::router::Router::register_static_handler("/api/ping",
                                             [](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
                                               (void)ctx;
                                               res.result(http::status::ok);
                                               res.set(http::field::content_type, "application/json");
                                               res.body() = R"({"ping":"pong"})";
                                             });

    std::cerr << "Listening on http://127.0.0.1:" << port << "/\n";
    std::cerr << "Static root: " << std::filesystem::weakly_canonical(www).string() << "\n";

    std::thread runner([&pool] { pool.run_blocking(); });
    runner.join();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
