/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * session and middleware Integration Example
 * Demonstrates how the enhanced middleware system works with sessions
 */

#include <chrono>
#include <foxhttp/foxhttp.hpp>
#include <iostream>
#include <thread>

using namespace foxhttp;

// Example middleware that demonstrates different execution results
class TestMiddleware : public middleware {
 public:
  std::string name() const override { return "TestMiddleware"; }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override {
    std::cout << "TestMiddleware: Processing " << ctx.path() << std::endl;

    // Simulate different behaviors based on path
    if (ctx.path() == "/error") {
      throw std::runtime_error("Simulated error for testing");
    } else if (ctx.path() == "/stop") {
      res.result(http::status::ok);
      res.set(http::field::content_type, "application/json");
      res.body() = R"({"message": "middleware stopped execution"})";
      // Don't call next() - this stops the chain
      return;
    } else if (ctx.path() == "/timeout") {
      // Simulate a long-running operation
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    next();
  }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override {
    std::cout << "TestMiddleware: Async processing " << ctx.path() << std::endl;

    if (ctx.path() == "/error") {
      callback(middleware_result::error, "Simulated async error");
      return;
    } else if (ctx.path() == "/stop") {
      res.result(http::status::ok);
      res.set(http::field::content_type, "application/json");
      res.body() = R"({"message": "Async middleware stopped execution"})";
      callback(middleware_result::stop, "");
      return;
    } else if (ctx.path() == "/timeout") {
      // Simulate async timeout
      std::thread([callback]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        callback(middleware_result::timeout, "Async operation timed out");
      }).detach();
      return;
    }

    // Simulate async work
    std::thread([next, callback]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      next();
      callback(middleware_result::continue_, "");
    }).detach();
  }
};

// Simulate a route handler
class MockRouteHandler {
 public:
  void handle_request(request_context &ctx, http::response<http::string_body> &res) {
    if (ctx.path() == "/hello") {
      res.result(http::status::ok);
      res.set(http::field::content_type, "application/json");
      res.body() = R"({"message": "Hello from route handler!", "path": ")" + ctx.path() + R"("})";
    } else if (ctx.path() == "/api/data") {
      res.result(http::status::ok);
      res.set(http::field::content_type, "application/json");
      res.body() = R"({"data": [1, 2, 3, 4, 5], "timestamp": )" +
                   std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                                      std::chrono::system_clock::now().time_since_epoch())
                                      .count()) +
                   "}";
    } else {
      res.result(http::status::not_found);
      res.set(http::field::content_type, "application/json");
      res.body() = R"({"error": "Route not found"})";
    }
  }
};

// Simulate session behavior
void simulate_session_processing(middleware_chain &chain, const std::string &path) {
  std::cout << "\n=== Processing Request: " << path << " ===" << std::endl;

  // Create mock request
  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target(path);
  req.set(http::field::host, "localhost");
  req.body() = "";
  req.prepare_payload();

  request_context ctx(req);
  http::response<http::string_body> res;
  MockRouteHandler handler;

  // Simulate the session's process_request logic
  res.version(req.version());
  res.keep_alive(req.keep_alive());

  // Execute middleware chain asynchronously (like in session.cpp)
  chain.execute_async(
      ctx, res, [&handler, &ctx, &res, path](middleware_result result, const std::string &error_message) {
        std::cout << "middleware chain completed with result: " << static_cast<int>(result);
        if (!error_message.empty()) {
          std::cout << ", error: " << error_message;
        }
        std::cout << std::endl;

        if (result == middleware_result::continue_) {
          // Call route handler (like in session.cpp)
          handler.handle_request(ctx, res);
        } else if (result == middleware_result::error) {
          res.result(http::status::internal_server_error);
          res.set(http::field::content_type, "application/json");
          res.body() = R"({"code":500,"message":"Internal Server Error","error":")" + error_message + R"("})";
        } else if (result == middleware_result::timeout) {
          res.result(http::status::gateway_timeout);
          res.set(http::field::content_type, "application/json");
          res.body() = R"({"code":504,"message":"Gateway Timeout"})";
        } else if (result == middleware_result::stop) {
          // middleware chain was stopped, response should already be set
          std::cout << "middleware chain was stopped, using middleware response" << std::endl;
        }

        res.prepare_payload();

        // Print response (simulating write_response)
        std::cout << "Response: " << res.result() << std::endl;
        std::cout << "Body: " << res.body() << std::endl;
      });
}

int main() {
  try {
    // Create IO context
    boost::asio::io_context io_context;

    // Create middleware chain
    middleware_chain chain(io_context);
    chain.enable_statistics(true);
    chain.set_global_timeout(std::chrono::milliseconds(3000));

    // Add test middleware
    chain.use(std::make_shared<TestMiddleware>());

    // Add utility middlewares
    std::cout << "=== session-middleware Integration Test ===" << std::endl;
    std::cout << "middleware chain:" << std::endl;
    for (const auto &name : chain.get_middleware_names()) {
      std::cout << "  - " << name << std::endl;
    }

    // Test different scenarios
    simulate_session_processing(chain, "/hello");
    simulate_session_processing(chain, "/api/data");
    simulate_session_processing(chain, "/error");
    simulate_session_processing(chain, "/stop");
    simulate_session_processing(chain, "/timeout");

    // Run IO context to process async operations
    std::cout << "\n=== Running async operations ===" << std::endl;
    io_context.run_for(std::chrono::milliseconds(5000));

    // Print statistics
    std::cout << "\n=== Final Statistics ===" << std::endl;
    chain.print_statistics();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
