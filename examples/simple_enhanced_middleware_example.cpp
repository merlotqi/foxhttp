/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * Simple Enhanced middleware Example - Demonstrates the improved middleware system
 */

#include <chrono>
#include <foxhttp/foxhttp.hpp>
#include <iostream>
#include <thread>

using namespace foxhttp;

int main() {
  try {
    // Create IO context
    boost::asio::io_context io_context;

    // Create middleware chain with IO context
    middleware_chain chain(io_context);

    // Enable statistics
    chain.enable_statistics(true);

    // Set global timeout
    chain.set_global_timeout(std::chrono::milliseconds(5000));

    // Print middleware names
    std::cout << "=== middleware Chain ===" << std::endl;
    for (const auto &name : chain.get_middleware_names()) {
      std::cout << "  - " << name << std::endl;
    }
    std::cout << std::endl;

    // Create test request
    http::request<http::string_body> req;
    req.method(http::verb::get);
    req.target("/api/hello");
    req.set(http::field::host, "localhost");
    req.body() = "";
    req.prepare_payload();

    request_context ctx(req);
    http::response<http::string_body> res;

    // Test synchronous execution
    std::cout << "=== Synchronous Execution ===" << std::endl;
    chain.execute(ctx, res);
    std::cout << "Response: " << res.result() << std::endl;
    std::cout << "Body: " << res.body() << std::endl;
    std::cout << std::endl;

    // Reset response
    res = http::response<http::string_body>();

    // Test asynchronous execution
    std::cout << "=== Asynchronous Execution ===" << std::endl;
    chain.execute_async(ctx, res, [&res](middleware_result result, const std::string &error) {
      std::cout << "Async execution completed with result: " << static_cast<int>(result);
      if (!error.empty()) {
        std::cout << ", error: " << error;
      }
      std::cout << std::endl;
    });

    // Run IO context to process async operations
    io_context.run_for(std::chrono::milliseconds(1000));

    std::cout << "Async Response: " << res.result() << std::endl;
    std::cout << "Async Body: " << res.body() << std::endl;
    std::cout << std::endl;

    // Print statistics
    std::cout << "=== Statistics ===" << std::endl;
    chain.print_statistics();

    // Test middleware removal
    std::cout << "=== Testing middleware Removal ===" << std::endl;
    chain.remove("CustomMiddleware");
    std::cout << "After removal:" << std::endl;
    for (const auto &name : chain.get_middleware_names()) {
      std::cout << "  - " << name << std::endl;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
