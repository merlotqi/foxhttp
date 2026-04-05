// Example: Error handling — demonstrates how to use error_callback for exception handling
//
// Build: cmake -S . -B build -DFOXHTTP_BUILD_EXAMPLES=ON && cmake --build build
// Run:   ./build/examples/08_error_handling
// Try:   curl -s http://127.0.0.1:8080/error to trigger an error

#include <csignal>
#include <foxhttp/foxhttp.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

#include "detail/router_dispatch_middleware.hpp"

namespace http = boost::beast::http;

std::atomic<bool> g_running{true};

// Simple error logger
class ErrorLogger {
public:
  void log_error(const std::exception_ptr& eptr, const std::string& context) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::cerr << "[Error Logger] " << std::ctime(&time);
    std::cerr << "  Context: " << context << "\n";
    
    if (eptr) {
      try {
        std::rethrow_exception(eptr);
      } catch (const std::exception& e) {
        std::cerr << "  Exception: " << e.what() << "\n";
      } catch (...) {
        std::cerr << "  Exception: Unknown exception\n";
      }
    }
    
    error_count_++;
  }
  
  size_t get_error_count() const { return error_count_; }
  
private:
  std::mutex mutex_;
  size_t error_count_{0};
};

int main() {
  const unsigned short port = 8080;
  ErrorLogger error_logger;

  try {
    foxhttp::server::IoContextPool pool(1);

    // Setup signal handlers
    auto sigs = std::make_shared<foxhttp::server::SignalSet>(pool.get_io_context());
    sigs->register_handler(SIGINT, [&](int, const std::string &) {
      std::cerr << "\n[Signal] Stopping...\n";
      pool.stop();
      g_running = false;
    });
#ifndef _WIN32
    sigs->register_handler(SIGTERM, [&](int, const std::string &) {
      pool.stop();
      g_running = false;
    });
#endif
    sigs->start();

    // Create server
    foxhttp::server::Server server(pool, port);
    server.use(std::make_shared<foxhttp::examples::router_dispatch_middleware>());

    // Register routes
    foxhttp::router::Router::register_static_handler("/",
      [](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
        (void)ctx;
        res.result(http::status::ok);
        res.set(http::field::content_type, "text/plain");
        res.body() = "FoxHttp example 08 — error handling.\n"
                     "Try: GET /error  GET /throw  GET /stats\n";
      });

    // Route that returns an error response
    foxhttp::router::Router::register_static_handler("/error",
      [](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
        (void)ctx;
        res.result(http::status::internal_server_error);
        res.set(http::field::content_type, "application/json");
        res.body() = R"({"error":"Simulated error","code":500})";
      });

    // Route that throws an exception (demonstrates exception handling)
    foxhttp::router::Router::register_static_handler("/throw",
      [&error_logger](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
        (void)ctx;
        try {
          throw std::runtime_error("Intentional exception for demonstration");
        } catch (...) {
          error_logger.log_error(std::current_exception(), "Route /throw");
          res.result(http::status::internal_server_error);
          res.set(http::field::content_type, "application/json");
          res.body() = R"({"error":"Exception caught and logged","code":500})";
        }
      });

    // Route that shows error statistics
    foxhttp::router::Router::register_static_handler("/stats",
      [&error_logger](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
        (void)ctx;
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = R"({"error_count":)" + std::to_string(error_logger.get_error_count()) + R"(})";
      });

    // Route that demonstrates error callback usage
    foxhttp::router::Router::register_static_handler("/callback",
      [&error_logger](foxhttp::server::RequestContext &ctx, http::response<http::string_body> &res) {
        (void)ctx;
        
        // This demonstrates how to use error callbacks in middleware
        // In a real application, you would set this up in your middleware
        auto error_callback = [&](const std::exception_ptr& eptr) {
          error_logger.log_error(eptr, "Middleware callback");
        };
        
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = R"({"message":"Error callback demonstration","error_count":)" + 
                     std::to_string(error_logger.get_error_count()) + R"(})";
      });

    std::cerr << "Listening on http://127.0.0.1:" << port << "/\n";
    std::cerr << "Try these endpoints:\n";
    std::cerr << "  curl http://127.0.0.1:" << port << "/\n";
    std::cerr << "  curl http://127.0.0.1:" << port << "/error\n";
    std::cerr << "  curl http://127.0.0.1:" << port << "/throw\n";
    std::cerr << "  curl http://127.0.0.1:" << port << "/stats\n";
    std::cerr << "  curl http://127.0.0.1:" << port << "/callback\n";
    std::cerr << "Press Ctrl+C to stop\n";

    // Run the server
    std::thread runner([&pool] { pool.run_blocking(); });
    
    // Monitor errors
    while (g_running) {
      std::this_thread::sleep_for(std::chrono::seconds(5));
      
      if (error_logger.get_error_count() > 0) {
        std::cerr << "[Monitor] Total errors logged: " << error_logger.get_error_count() << "\n";
      }
    }
    
    runner.join();
    std::cerr << "[Server] Stopped. Total errors: " << error_logger.get_error_count() << "\n";
    
  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
