/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * Simple Enhanced Middleware Example - Demonstrates the improved middleware system
 */

#include <chrono>
#include <foxhttp/foxhttp.hpp>
#include <iostream>
#include <thread>

using namespace foxhttp;

int main()
{
    try
    {
        // Create IO context
        boost::asio::io_context io_context;

        // Create middleware chain with IO context
        MiddlewareChain chain(io_context);

        // Enable statistics
        chain.enable_statistics(true);

        // Set global timeout
        chain.set_global_timeout(std::chrono::milliseconds(5000));

        // Add middlewares using the utility functions
        chain.use(middleware_utils::create_logger("RequestLogger"));
        chain.use(middleware_utils::create_cors("*"));
        chain.use(middleware_utils::create_response_time());

        // Add a custom middleware using the builder pattern
        auto custom_middleware = MiddlewareBuilder()
                                         .name("CustomMiddleware")
                                         .priority(middleware_priority::normal)
                                         .sync([](RequestContext &ctx, http::response<http::string_body> &res,
                                                  std::function<void()> next) {
                                             std::cout << "Custom middleware processing: " << ctx.path() << std::endl;
                                             next();
                                         })
                                         .build();

        chain.use(custom_middleware);

        // Add a route handler
        auto route_handler =
                MiddlewareBuilder()
                        .name("RouteHandler")
                        .priority(middleware_priority::low)
                        .sync([](RequestContext &ctx, http::response<http::string_body> &res,
                                 std::function<void()> next) {
                            if (ctx.path() == "/api/hello")
                            {
                                res.result(http::status::ok);
                                res.set(http::field::content_type, "application/json");
                                res.body() =
                                        R"({"message": "Hello from enhanced FoxHTTP!", "features": ["async", "priority", "statistics", "timeout"]})";
                            }
                            else
                            {
                                res.result(http::status::not_found);
                                res.set(http::field::content_type, "text/plain");
                                res.body() = "Not found";
                            }
                        })
                        .build();

        chain.use(route_handler);

        // Print middleware names
        std::cout << "=== Middleware Chain ===" << std::endl;
        for (const auto &name: chain.get_middleware_names())
        {
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

        RequestContext ctx(req);
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
            if (!error.empty())
            {
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
        std::cout << "=== Testing Middleware Removal ===" << std::endl;
        chain.remove("CustomMiddleware");
        std::cout << "After removal:" << std::endl;
        for (const auto &name: chain.get_middleware_names())
        {
            std::cout << "  - " << name << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
