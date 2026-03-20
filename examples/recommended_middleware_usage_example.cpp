#include <chrono>
#include <foxhttp/foxhttp.hpp>
#include <iostream>
#include <thread>

using namespace foxhttp;

// Custom middleware using proper inheritance
class CustomAuthMiddleware : public middleware {
 public:
  explicit CustomAuthMiddleware(const std::string &secret_key) : secret_key_(secret_key) {}

  std::string name() const override { return "CustomAuthMiddleware"; }
  middleware_priority priority() const override { return middleware_priority::high; }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override {
    std::string auth_header = ctx.header("Authorization");

    if (auth_header.empty()) {
      res.result(http::status::unauthorized);
      res.set(http::field::content_type, "application/json");
      res.body() = R"({"error": "Authorization header required"})";
      return;
    }

    if (auth_header != "Bearer " + secret_key_) {
      res.result(http::status::forbidden);
      res.set(http::field::content_type, "application/json");
      res.body() = R"({"error": "Invalid authorization token"})";
      return;
    }

    std::cout << "Authentication successful for: " << ctx.path() << std::endl;
    next();
  }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override {
    std::string auth_header = ctx.header("Authorization");

    if (auth_header.empty()) {
      res.result(http::status::unauthorized);
      res.set(http::field::content_type, "application/json");
      res.body() = R"({"error": "Authorization header required"})";
      callback(middleware_result::stop, "");
      return;
    }

    // Simulate async authentication check
    std::thread([auth_header, secret_key = secret_key_, next, callback]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));

      if (auth_header != "Bearer " + secret_key) {
        callback(middleware_result::error, "Invalid authorization token");
        return;
      }

      std::cout << "Async authentication successful" << std::endl;
      next();
      callback(middleware_result::continue_, "");
    }).detach();
  }

 private:
  std::string secret_key_;
};

// Route handler middleware
class RouteHandlerMiddleware : public priority_middleware<middleware_priority::low> {
 public:
  std::string name() const override { return "RouteHandlerMiddleware"; }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override {
    if (ctx.path() == "/api/hello") {
      res.result(http::status::ok);
      res.set(http::field::content_type, "application/json");
      res.body() = R"({"message": "Hello from FoxHTTP!", "path": ")" + ctx.path() + R"("})";
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

int main() {
  try {
    // Create IO context
    boost::asio::io_context io_context;

    // Create middleware chain
    middleware_chain chain(io_context);
    chain.enable_statistics(true);
    chain.set_global_timeout(std::chrono::milliseconds(5000));

    std::cout << "=== Recommended middleware Usage ===" << std::endl;

    // Method 1: Use specific middleware classes (RECOMMENDED)
    std::cout << "\n1. Using specific middleware classes:" << std::endl;
    chain.use(middleware_factories::create_logger_middleware("RequestLogger"));
    chain.use(middleware_factories::create_cors_middleware("*"));
    chain.use(std::make_shared<CustomAuthMiddleware>("secret-key-123"));
    chain.use(std::make_shared<RouteHandlerMiddleware>());
    chain.use(middleware_factories::create_response_time_middleware());

    // Method 2: Use utility functions for simple cases (LEGACY)
    std::cout << "\n2. Using utility functions (legacy, for simple cases):" << std::endl;
    // chain.use(middleware_utils::create_logger("SimpleLogger"));
    // chain.use(middleware_utils::create_cors("*"));

    // Method 3: Use SimpleFunctionalMiddleware for quick prototyping (DEPRECATED)
    std::cout << "\n3. Using SimpleFunctionalMiddleware (deprecated, for quick prototyping):" << std::endl;
    // auto quick_middleware = std::make_shared<SimpleFunctionalMiddleware>("QuickMW",
    //     [](const request_context& ctx, auto& res, auto next) {
    //         std::cout << "Quick middleware: " << ctx.path() << std::endl;
    //         next();
    //     });
    // chain.use(quick_middleware);

    // Print middleware chain
    std::cout << "\nMiddleware chain:" << std::endl;
    for (const auto &name : chain.get_middleware_names()) {
      std::cout << "  - " << name << std::endl;
    }

    // Test requests
    auto test_request = [&chain](const std::string &path, const std::string &auth_header = "") {
      std::cout << "\n=== Testing: " << path << " ===" << std::endl;

      http::request<http::string_body> req;
      req.method(http::verb::get);
      req.target(path);
      req.set(http::field::host, "localhost");
      if (!auth_header.empty()) {
        req.set(http::field::authorization, auth_header);
      }
      req.body() = "";
      req.prepare_payload();

      request_context ctx(req);
      http::response<http::string_body> res;

      chain.execute_async(ctx, res, [&res](middleware_result result, const std::string &error_message) {
        std::cout << "Result: " << static_cast<int>(result);
        if (!error_message.empty()) {
          std::cout << ", Error: " << error_message;
        }
        std::cout << std::endl;
        std::cout << "Response: " << res.result() << std::endl;
        std::cout << "Body: " << res.body() << std::endl;
      });
    };

    // Test different scenarios
    test_request("/api/hello", "Bearer secret-key-123");
    test_request("/api/data", "Bearer secret-key-123");
    test_request("/api/hello");                      // No auth
    test_request("/api/hello", "Bearer wrong-key");  // Wrong auth
    test_request("/api/unknown");                    // Unknown route

    // Run async operations
    std::cout << "\n=== Running async operations ===" << std::endl;
    io_context.run_for(std::chrono::milliseconds(2000));

    // Print statistics
    std::cout << "\n=== Final Statistics ===" << std::endl;
    chain.print_statistics();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
