/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * SPDLog middleware Example
 * Demonstrates the enhanced logging capabilities with spdlog
 */

#include <chrono>
#include <foxhttp/foxhttp.hpp>
#include <iostream>
#include <thread>

using namespace foxhttp;

// Test middleware that generates different types of requests
class TestMiddleware : public middleware {
 public:
  std::string name() const override { return "TestMiddleware"; }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override {
    // Simulate different request types
    if (ctx.path() == "/error") {
      throw std::runtime_error("Simulated error for logging test");
    } else if (ctx.path() == "/slow") {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } else if (ctx.path() == "/large") {
      res.body() = std::string(10000, 'A');  // Large response
    }

    next();
  }
};

int main() {
  try {
    // Create IO context
    boost::asio::io_context io_context;

    // Create middleware chain
    middleware_chain chain(io_context);
    chain.enable_statistics(true);

    std::cout << "=== SPDLog middleware Examples ===" << std::endl;

    // Example 1: Simple console logging
    std::cout << "\n1. Simple Console Logging:" << std::endl;
    auto simple_logger = std::make_shared<logger_middleware>("SimpleLogger", log_level::info, log_format::simple);

    // Example 2: Detailed logging with file output
    std::cout << "\n2. Detailed Logging with File Output:" << std::endl;
    auto detailed_logger = std::make_shared<logger_middleware>("DetailedLogger", log_level::debug, log_format::detailed,
                                                               "foxhttp.log"  // Log to file
    );

    // Example 3: JSON structured logging
    std::cout << "\n3. JSON Structured Logging:" << std::endl;
    auto json_logger =
        std::make_shared<logger_middleware>("JsonLogger", log_level::info, log_format::json, "foxhttp.json.log");

    // Example 4: Apache Common Log Format
    std::cout << "\n4. Apache Common Log Format:" << std::endl;
    auto apache_logger =
        std::make_shared<logger_middleware>("ApacheLogger", log_level::info, log_format::apache, "access.log");

    // Example 5: High-performance file-only logging
    std::cout << "\n5. High-Performance File-Only Logging:" << std::endl;
    auto perf_logger =
        std::make_shared<logger_middleware>("PerfLogger", log_level::warn, log_format::simple, "performance.log",
                                            false  // Disable console output
        );

    // Add test middleware
    chain.use(std::make_shared<TestMiddleware>());

    // Test different loggers
    auto test_logger = [&chain](std::shared_ptr<logger_middleware> logger, const std::string &name) {
      std::cout << "\n--- Testing " << name << " ---" << std::endl;

      // Clear existing middlewares and add the test logger
      chain.clear();
      chain.use(logger);
      chain.use(std::make_shared<TestMiddleware>());

      // Test different request types
      auto test_request = [&chain](const std::string &path, const std::string &user_agent = "TestAgent/1.0") {
        http::request<http::string_body> req;
        req.method(http::verb::get);
        req.target(path);
        req.set(http::field::host, "localhost");
        req.set(http::field::user_agent, user_agent);
        req.set("X-Forwarded-For", "192.168.1.100");
        req.body() = "";
        req.prepare_payload();

        request_context ctx(req);
        http::response<http::string_body> res;

        try {
          chain.execute(ctx, res);
        } catch (const std::exception &e) {
          // Expected for error test
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      };

      // Test various scenarios
      test_request("/api/hello");
      test_request("/api/data");
      test_request("/slow");
      test_request("/large");
      test_request("/error");  // This will throw an exception
    };

    // Test each logger
    test_logger(simple_logger, "Simple Logger");
    test_logger(detailed_logger, "Detailed Logger");
    test_logger(json_logger, "JSON Logger");
    test_logger(apache_logger, "Apache Logger");
    test_logger(perf_logger, "Performance Logger");

    // Example 6: Runtime configuration
    std::cout << "\n6. Runtime Configuration:" << std::endl;
    auto configurable_logger = std::make_shared<logger_middleware>("ConfigurableLogger");

    // Change log level
    configurable_logger->set_log_level(log_level::debug);
    std::cout << "Changed log level to DEBUG" << std::endl;

    // Change log format
    configurable_logger->set_log_format(log_format::json);
    std::cout << "Changed log format to JSON" << std::endl;

    // Enable file logging
    configurable_logger->set_log_file("runtime_config.log");
    std::cout << "Enabled file logging" << std::endl;

    // Disable console output
    configurable_logger->set_console_enabled(false);
    std::cout << "Disabled console output" << std::endl;

    // Test the configurable logger
    chain.clear();
    chain.use(configurable_logger);
    chain.use(std::make_shared<TestMiddleware>());

    http::request<http::string_body> req;
    req.method(http::verb::post);
    req.target("/api/test");
    req.set(http::field::host, "localhost");
    req.set(http::field::user_agent, "ConfigTest/1.0");
    req.body() = "test data";
    req.prepare_payload();

    request_context ctx(req);
    http::response<http::string_body> res;
    chain.execute(ctx, res);

    // Example 7: Async logging
    std::cout << "\n7. Async Logging Test:" << std::endl;
    auto async_logger = std::make_shared<logger_middleware>("AsyncLogger", log_level::info, log_format::detailed);

    chain.clear();
    chain.use(async_logger);
    chain.use(std::make_shared<TestMiddleware>());

    chain.execute_async(ctx, res, [](middleware_result result, const std::string &error_message) {
      std::cout << "Async request completed with result: " << static_cast<int>(result) << std::endl;
    });

    // Run async operations
    io_context.run_for(std::chrono::milliseconds(1000));

    // Print statistics
    std::cout << "\n=== Final Statistics ===" << std::endl;
    chain.print_statistics();

    std::cout << "\n=== Log Files Created ===" << std::endl;
    std::cout << "Check the following log files:" << std::endl;
    std::cout << "- foxhttp.log (detailed format)" << std::endl;
    std::cout << "- foxhttp.json.log (JSON format)" << std::endl;
    std::cout << "- access.log (Apache format)" << std::endl;
    std::cout << "- performance.log (performance logs)" << std::endl;
    std::cout << "- runtime_config.log (runtime configuration test)" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
