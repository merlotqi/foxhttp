// Example: HTTP Client timeout configuration
//
// This example demonstrates how to configure timeouts for HTTP client requests.
// Build: cmake -S . -B build -DFOXHTTP_BUILD_EXAMPLES=ON && cmake --build build
// Run:   ./build/examples/09_client_timeout

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <foxhttp/foxhttp.hpp>
#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> demo_default_timeout() {
  std::cout << "\n=== Demo 1: Default Timeout (30s) ===\n";

  foxhttp::client::http_client client(co_await asio::this_coro::executor,
                                      "http://httpbin.org");

  try {
    auto res = co_await client.get("/get").as_awaitable();
    std::cout << "Response status: " << res.result_int() << "\n";
  } catch (const std::exception& e) {
    std::cout << "Error: " << e.what() << "\n";
  }
}

asio::awaitable<void> demo_custom_client_timeout() {
  std::cout << "\n=== Demo 2: Custom Client Timeout ===\n";

  foxhttp::client::http_client client(co_await asio::this_coro::executor,
                                      "http://httpbin.org");

  // Configure client-wide timeout options
  foxhttp::client::client_options opts;
  opts.set_connection_timeout(std::chrono::milliseconds(5000))  // 5 seconds
      .set_request_timeout(std::chrono::milliseconds(10000))    // 10 seconds
      .set_user_agent("FoxHttp-Timeout-Example/1.0");

  client.set_options(opts);

  try {
    auto res = co_await client.get("/get").as_awaitable();
    std::cout << "Response status: " << res.result_int() << "\n";
  } catch (const std::exception& e) {
    std::cout << "Error: " << e.what() << "\n";
  }
}

asio::awaitable<void> demo_per_request_timeout() {
  std::cout << "\n=== Demo 3: Per-Request Timeout ===\n";

  foxhttp::client::http_client client(co_await asio::this_coro::executor,
                                      "http://httpbin.org");

  try {
    // Override timeout for this specific request
    auto res = co_await client.get("/get")
                   .connection_timeout(std::chrono::milliseconds(3000))
                   .request_timeout(std::chrono::milliseconds(5000))
                   .as_awaitable();
    std::cout << "Response status: " << res.result_int() << "\n";
  } catch (const std::exception& e) {
    std::cout << "Error: " << e.what() << "\n";
  }
}

asio::awaitable<void> demo_timeout_with_options() {
  std::cout << "\n=== Demo 4: Request Timeout with Options ===\n";

  foxhttp::client::http_client client(co_await asio::this_coro::executor,
                                      "http://httpbin.org");

  // Create timeout options for this request
  foxhttp::client::request_timeout_options timeout_opts;
  timeout_opts.set_connection_timeout(std::chrono::milliseconds(2000))
      .set_request_timeout(std::chrono::milliseconds(4000));

  try {
    auto res = co_await client.get("/get")
                   .timeout(timeout_opts)
                   .as_awaitable();
    std::cout << "Response status: " << res.result_int() << "\n";
  } catch (const std::exception& e) {
    std::cout << "Error: " << e.what() << "\n";
  }
}

asio::awaitable<void> demo_short_timeout() {
  std::cout << "\n=== Demo 5: Very Short Timeout (Expected to Fail) ===\n";

  foxhttp::client::http_client client(co_await asio::this_coro::executor,
                                      "http://httpbin.org");

  try {
    // This will likely timeout
    auto res = co_await client.get("/delay/5")
                   .connection_timeout(std::chrono::milliseconds(100))
                   .request_timeout(std::chrono::milliseconds(200))
                   .as_awaitable();
    std::cout << "Response status: " << res.result_int() << "\n";
  } catch (const std::exception& e) {
    std::cout << "Expected timeout error: " << e.what() << "\n";
  }
}

int main() {
  try {
    asio::io_context ioc(1);

    asio::co_spawn(ioc, demo_default_timeout, asio::detached);
    asio::co_spawn(ioc, demo_custom_client_timeout, asio::detached);
    asio::co_spawn(ioc, demo_per_request_timeout, asio::detached);
    asio::co_spawn(ioc, demo_timeout_with_options, asio::detached);
    asio::co_spawn(ioc, demo_short_timeout, asio::detached);

    ioc.run();

    std::cout << "\n=== All demos completed ===\n";
  } catch (const std::exception& e) {
    std::cerr << "Fatal error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
