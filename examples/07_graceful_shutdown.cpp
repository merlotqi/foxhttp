// Example: Graceful shutdown — demonstrates how to properly stop a server
//
// Build: cmake -S . -B build -DFOXHTTP_BUILD_EXAMPLES=ON && cmake --build build
// Run:   ./build/examples/07_graceful_shutdown
// Try:   curl -s http://127.0.0.1:8080/ and then press Ctrl+C

#include <csignal>
#include <foxhttp/foxhttp.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

#include "detail/router_dispatch_middleware.hpp"

namespace http = boost::beast::http;

std::atomic<bool> g_running{true};
std::atomic<bool> g_shutdown_requested{false};

int main() {
  const unsigned short port = 8080;

  try {
    foxhttp::io_context_pool pool(1);

    // Create server
    foxhttp::server server(pool, port);
    server.use(std::make_shared<foxhttp::examples::router_dispatch_middleware>());

    // Setup signal handlers for graceful shutdown
    auto sigs = std::make_shared<foxhttp::signal_set>(pool.get_io_context());
    
    sigs->register_handler(SIGINT, [&](int, const std::string &) {
      std::cerr << "\n[Signal] SIGINT received, initiating graceful shutdown...\n";
      g_shutdown_requested = true;
      
      // Stop accepting new connections
      server.stop();
      std::cerr << "[Server] Stopped accepting new connections\n";
      
      // In a real application, you would wait for existing requests to complete
      std::cerr << "[Server] Waiting for existing requests to complete...\n";
      std::this_thread::sleep_for(std::chrono::seconds(2));
      
      // Stop the io_context pool
      pool.stop();
      g_running = false;
    });
    
#ifndef _WIN32
    sigs->register_handler(SIGTERM, [&](int, const std::string &) {
      std::cerr << "\n[Signal] SIGTERM received, initiating graceful shutdown...\n";
      g_shutdown_requested = true;
      server.stop();
      pool.stop();
      g_running = false;
    });
#endif
    sigs->start();

    // Register some example routes
    foxhttp::router::register_static_handler("/",
      [](foxhttp::request_context &ctx, http::response<http::string_body> &res) {
        (void)ctx;
        res.result(http::status::ok);
        res.set(http::field::content_type, "text/plain");
        res.body() = "FoxHttp example 07 — graceful shutdown.\n"
                     "Press Ctrl+C to see graceful shutdown in action.\n";
      });

    foxhttp::router::register_static_handler("/slow",
      [](foxhttp::request_context &ctx, http::response<http::string_body> &res) {
        (void)ctx;
        // Simulate a slow request
        std::this_thread::sleep_for(std::chrono::seconds(5));
        res.result(http::status::ok);
        res.set(http::field::content_type, "text/plain");
        res.body() = "This was a slow request (5 seconds).\n";
      });

    foxhttp::router::register_static_handler("/health",
      [](foxhttp::request_context &ctx, http::response<http::string_body> &res) {
        (void)ctx;
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = R"({"status":"ok","shutdown_requested":)" + 
                     std::string(g_shutdown_requested ? "true" : "false") + "}";
      });

    std::cerr << "Listening on http://127.0.0.1:" << port << "/\n";
    std::cerr << "Press Ctrl+C to trigger graceful shutdown\n";
    std::cerr << "Try: curl http://127.0.0.1:" << port << "/slow to see request completion during shutdown\n";

    // Run the server
    std::thread runner([&pool] { pool.run_blocking(); });
    
    // Monitor shutdown status
    while (g_running) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      
      if (g_shutdown_requested) {
        std::cerr << "[Monitor] Shutdown in progress...\n";
      }
    }
    
    runner.join();
    std::cerr << "[Server] Graceful shutdown complete\n";
    
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
