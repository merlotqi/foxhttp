// Example: Instance-based router with HTTP method routing (new API).
// This example demonstrates:
// 1. Using a Router instance instead of the global singleton
// 2. HTTP method-based routing (GET, POST, etc.)
// 3. Automatic 405 Method Not Allowed responses with Allow header
//
// Run: ./build/examples/10_instance_router
// Try: curl -s http://127.0.0.1:8090/
//      curl -s http://127.0.0.1:8090/api/users
//      curl -s -X POST http://127.0.0.1:8090/api/users
//      curl -s -X PUT http://127.0.0.1:8090/api/users  (405 Method Not Allowed)

#include <foxhttp/foxhttp.hpp>
#include <iostream>
#include <thread>

#include "detail/router_dispatch_middleware.hpp"

namespace http = boost::beast::http;

int main() {
  const unsigned short port = 8090;

  try {
    foxhttp::server::IoContextPool pool(1);
    auto sigs = std::make_shared<foxhttp::server::SignalSet>(pool.get_io_context());
    sigs->register_handler(SIGINT, [&pool](int, const std::string &) { pool.stop(); });
    sigs->start();

    // --- NEW API: Create a router instance ---
    auto my_router = std::make_shared<foxhttp::router::Router>();

    // Root path - any method
    my_router->add_static_route(
        "/", [](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
          (void)ctx;
          res.result(http::status::ok);
          res.set(http::field::content_type, "text/plain");
          res.body() = "Example 10 - Instance-based router with HTTP method routing.\n"
                       "Try: GET /api/users, POST /api/users\n";
        });

    // HTTP method-based routing on /api/users
    my_router->add_static_route(
        "/api/users", http::verb::get,
        [](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
          (void)ctx;
          res.result(http::status::ok);
          res.set(http::field::content_type, "application/json");
          res.body() = R"({"action":"list_users","method":"GET"})";
        });

    my_router->add_static_route(
        "/api/users", http::verb::post,
        [](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
          (void)ctx;
          res.result(http::status::ok);
          res.set(http::field::content_type, "application/json");
          res.body() = R"({"action":"create_user","method":"POST"})";
        });

    // Dynamic route with method
    my_router->add_dynamic_route(
        "/api/users/{id}", http::verb::get,
        [](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
          res.result(http::status::ok);
          res.set(http::field::content_type, "application/json");
          res.body() = std::string(R"({"action":"get_user","id":")") + ctx.path_parameter("id") + "\"}";
        });

    my_router->add_dynamic_route(
        "/api/users/{id}", http::verb::delete_,
        [](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
          res.result(http::status::ok);
          res.set(http::field::content_type, "application/json");
          res.body() = std::string(R"({"action":"delete_user","id":")") + ctx.path_parameter("id") + "\"}";
        });

    foxhttp::server::Server server(pool, port);

    // Use instance-based router dispatch middleware
    auto dispatch = std::make_shared<foxhttp::examples::router_dispatch_middleware>(my_router);
    server.use(dispatch);

    std::cerr << "Listening on http://127.0.0.1:" << port << "/ (Ctrl+C to stop)\n";
    std::cerr << "Using instance-based router with HTTP method routing\n";
    std::cerr << "Try:\n";
    std::cerr << "  curl http://127.0.0.1:" << port << "//\n";
    std::cerr << "  curl http://127.0.0.1:" << port << "/api/users\n";
    std::cerr << "  curl -X POST http://127.0.0.1:" << port << "/api/users\n";
    std::cerr << "  curl -X PUT http://127.0.0.1:" << port << "/api/users  (405)\n";

    std::thread runner([&pool] { pool.run_blocking(); });
    runner.join();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
