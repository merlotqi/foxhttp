/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * Header Method Test
 * Tests the new header() method with default values
 */

#include <foxhttp/foxhttp.hpp>
#include <iostream>

using namespace foxhttp;

int main() {
  try {
    // Create a test HTTP request
    http::request<http::string_body> req;
    req.method(http::verb::get);
    req.target("/api/test");
    req.set(http::field::host, "localhost");
    req.set(http::field::user_agent, "TestAgent/1.0");
    req.set(http::field::referer, "https://example.com/page1");
    req.set("X-Forwarded-For", "192.168.1.100");
    req.set("X-Custom-Header", "custom-value");
    req.body() = "";
    req.prepare_payload();

    // Create request context
    request_context ctx(req);

    std::cout << "=== Header Method Test ===" << std::endl;

    // Test existing headers
    std::cout << "\n1. Testing existing headers:" << std::endl;
    std::cout << "User-Agent: " << ctx.header("User-Agent") << std::endl;
    std::cout << "Referer: " << ctx.header("Referer") << std::endl;
    std::cout << "X-Forwarded-For: " << ctx.header("X-Forwarded-For") << std::endl;
    std::cout << "X-Custom-Header: " << ctx.header("X-Custom-Header") << std::endl;

    // Test with default values for existing headers
    std::cout << "\n2. Testing existing headers with default values:" << std::endl;
    std::cout << "User-Agent (default 'unknown'): " << ctx.header("User-Agent", "unknown") << std::endl;
    std::cout << "Referer (default '-'): " << ctx.header("Referer", "-") << std::endl;
    std::cout << "X-Forwarded-For (default 'unknown'): " << ctx.header("X-Forwarded-For", "unknown") << std::endl;

    // Test non-existing headers
    std::cout << "\n3. Testing non-existing headers:" << std::endl;
    std::cout << "Authorization (no default): " << ctx.header("Authorization") << std::endl;
    std::cout << "Authorization (default 'none'): " << ctx.header("Authorization", "none") << std::endl;
    std::cout << "X-Real-IP (default 'unknown'): " << ctx.header("X-Real-IP", "unknown") << std::endl;
    std::cout << "Content-Type (default 'text/plain'): " << ctx.header("Content-Type", "text/plain") << std::endl;

    // Test with http::field enum
    std::cout << "\n4. Testing with http::field enum:" << std::endl;
    std::cout << "Host (no default): " << ctx.header(http::field::host) << std::endl;
    std::cout << "Host (default 'localhost'): " << ctx.header(http::field::host, "localhost") << std::endl;
    std::cout << "Content-Type (no default): " << ctx.header(http::field::content_type) << std::endl;
    std::cout << "Content-Type (default 'application/json'): "
              << ctx.header(http::field::content_type, "application/json") << std::endl;

    // Test Apache-style logging
    std::cout << "\n5. Testing Apache-style logging format:" << std::endl;
    std::string client_ip = ctx.header("X-Forwarded-For", ctx.header("X-Real-IP", "-"));
    std::string referer = ctx.header("Referer", "-");
    std::string user_agent = ctx.header("User-Agent", "-");

    std::cout << "Client IP: " << client_ip << std::endl;
    std::cout << "Referer: " << referer << std::endl;
    std::cout << "User-Agent: " << user_agent << std::endl;

    // Simulate Apache log format
    std::cout << "\n6. Simulated Apache log entry:" << std::endl;
    std::cout << client_ip << " - - [25/Dec/2023:10:30:56 +0000] \"GET /api/test HTTP/1.1\" 200 1024 \"" << referer
              << "\" \"" << user_agent << "\"" << std::endl;

    // Test case sensitivity
    std::cout << "\n7. Testing case sensitivity:" << std::endl;
    std::cout << "user-agent (lowercase): " << ctx.header("user-agent", "not-found") << std::endl;
    std::cout << "USER-AGENT (uppercase): " << ctx.header("USER-AGENT", "not-found") << std::endl;
    std::cout << "User-Agent (mixed case): " << ctx.header("User-Agent", "not-found") << std::endl;

    std::cout << "\n=== Test completed successfully! ===" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
