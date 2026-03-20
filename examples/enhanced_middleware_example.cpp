#include <chrono>
#include <foxhttp/foxhttp.hpp>
#include <iostream>
#include <thread>

using namespace foxhttp;

// Example middleware with high priority
class LoggerMiddleware : public priority_middleware<middleware_priority::high> {
 public:
  std::string name() const override { return "LoggerMiddleware"; }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override {
    auto start = std::chrono::steady_clock::now();

    std::cout << "[" << std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count() << "] "
              << ctx.method_string() << " " << ctx.path() << std::endl;

    next();

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Request processed in " << duration.count() << " μs" << std::endl;
  }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override {
    auto start = std::chrono::steady_clock::now();

    std::cout << "[" << std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count() << "] "
              << ctx.method_string() << " " << ctx.path() << " (async)" << std::endl;

    // Simulate async work
    std::thread([start, next, callback]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      next();
      callback(middleware_result::continue_, "");
    }).detach();
  }
};

// Example middleware with normal priority and timeout
class AuthMiddleware : public middleware {
 public:
  std::string name() const override { return "AuthMiddleware"; }
  std::chrono::milliseconds timeout() const override { return std::chrono::milliseconds(1000); }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override {
    std::string auth_header = ctx.header("Authorization");

    if (auth_header.empty()) {
      res.result(http::status::unauthorized);
      res.set(http::field::content_type, "text/plain");
      res.body() = "Authorization required";
      return;
    }

    if (auth_header != "Bearer valid-token") {
      res.result(http::status::forbidden);
      res.set(http::field::content_type, "text/plain");
      res.body() = "Invalid token";
      return;
    }

    std::cout << "Authentication successful" << std::endl;
    next();
  }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override {
    std::string auth_header = ctx.header("Authorization");

    if (auth_header.empty()) {
      res.result(http::status::unauthorized);
      res.set(http::field::content_type, "text/plain");
      res.body() = "Authorization required";
      callback(middleware_result::stop, "");
      return;
    }

    // Simulate async authentication check
    std::thread([auth_header, next, callback]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));

      if (auth_header != "Bearer valid-token") {
        callback(middleware_result::error, "Invalid token");
        return;
      }

      std::cout << "Async authentication successful" << std::endl;
      next();
      callback(middleware_result::continue_, "");
    }).detach();
  }
};

// Example middleware with conditional execution
class RateLimitMiddleware : public middleware {
 public:
  std::string name() const override { return "RateLimitMiddleware"; }

  bool should_execute(request_context &ctx) const override {
    // Only apply rate limiting to POST requests
    return ctx.method() == http::verb::post;
  }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override {
    std::cout << "Rate limiting applied to POST request" << std::endl;
    next();
  }
};

// Example middleware with low priority
class ResponseMiddleware : public priority_middleware<middleware_priority::low> {
 public:
  std::string name() const override { return "ResponseMiddleware"; }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override {
    next();

    // Add response headers after processing
    res.set(http::field::server, "FoxHTTP/1.0");
    res.set("X-Processed-By", "FoxHTTP-middleware");

    std::cout << "Response headers added" << std::endl;
  }
};

// Example route handler
class RouteHandler : public middleware {
 public:
  std::string name() const override { return "RouteHandler"; }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override {
    if (ctx.path() == "/api/hello") {
      res.result(http::status::ok);
      res.set(http::field::content_type, "application/json");
      res.body() = R"({"message": "Hello from FoxHTTP!", "timestamp": )" +
                   std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                                      std::chrono::system_clock::now().time_since_epoch())
                                      .count()) +
                   "}";
      std::cout << "Route handler executed" << std::endl;
    } else {
      res.result(http::status::not_found);
      res.set(http::field::content_type, "text/plain");
      res.body() = "Not found";
    }
  }
};

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

    // Set error handler
    chain.set_error_handler([](request_context &ctx, http::response<http::string_body> &res, const std::exception &e) {
      std::cout << "Error in middleware: " << e.what() << std::endl;
      res.result(http::status::internal_server_error);
      res.set(http::field::content_type, "text/plain");
      res.body() = "Internal server error";
    });

    // Set timeout handler
    chain.set_timeout_handler([](request_context &ctx, http::response<http::string_body> &res) {
      std::cout << "Request timeout" << std::endl;
      res.result(http::status::gateway_timeout);
      res.set(http::field::content_type, "text/plain");
      res.body() = "Request timeout";
    });

    // Add middlewares (they will be automatically sorted by priority)
    chain.use(std::make_shared<AuthMiddleware>());
    chain.use(std::make_shared<RateLimitMiddleware>());
    chain.use(std::make_shared<RouteHandler>());
    chain.use(std::make_shared<ResponseMiddleware>());

    // Print middleware names
    std::cout << "middleware chain:" << std::endl;
    for (const auto &name : chain.get_middleware_names()) {
      std::cout << "  - " << name << std::endl;
    }
    std::cout << std::endl;

    // Create test request
    http::request<http::string_body> req;
    req.method(http::verb::get);
    req.target("/api/hello");
    req.set(http::field::host, "localhost");
    req.set(http::field::authorization, "Bearer valid-token");
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

    // Test conditional middleware
    std::cout << "=== Testing Conditional middleware ===" << std::endl;
    http::request<http::string_body> post_req;
    post_req.method(http::verb::post);
    post_req.target("/api/data");
    post_req.set(http::field::host, "localhost");
    post_req.set(http::field::authorization, "Bearer valid-token");
    post_req.body() = R"({"data": "test"})";
    post_req.prepare_payload();

    request_context post_ctx(post_req);
    http::response<http::string_body> post_res;

    chain.execute(post_ctx, post_res);
    std::cout << "POST Response: " << post_res.result() << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
