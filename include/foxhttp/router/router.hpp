#pragma once

#include <foxhttp/handler/callable_api_handler.hpp>
#include <foxhttp/router/route_table.hpp>
#include <foxhttp/server/request_context.hpp>
#include <memory>
#include <string>
#include <utility>

namespace foxhttp::router {

using ApiHandler = handler::ApiHandler;

/// Router class for managing route registration and resolution.
/// Supports both instance-based usage (recommended) and global singleton (backward compatible).
class Router {
 public:
  /// Construct a router with its own route table
  explicit Router(std::shared_ptr<RouteTable> table = nullptr);

  /// Register a static route handler (matches any HTTP method)
  void add_static_route(const std::string &path, std::shared_ptr<ApiHandler> handler);

  /// Register a static route handler for a specific HTTP method
  void add_static_route(const std::string &path, http::verb method, std::shared_ptr<ApiHandler> handler);

  /// Register a dynamic route handler (e.g. "/api/users/{id}", matches any method)
  void add_dynamic_route(const std::string &pattern, std::shared_ptr<ApiHandler> handler);

  /// Register a dynamic route handler for a specific HTTP method
  void add_dynamic_route(const std::string &pattern, http::verb method, std::shared_ptr<ApiHandler> handler);

  /// Resolve route for the given path and context (matches any method, backward compatible)
  std::shared_ptr<ApiHandler> resolver(const std::string &path, server::RequestContext &ctx) const;

  /// Resolve route with HTTP method matching
  /// Returns pair of handler and list of allowed methods (for 405 response)
  std::pair<std::shared_ptr<ApiHandler>, std::vector<http::verb>> resolver_with_method(
      const std::string &path, http::verb method, server::RequestContext &ctx) const;

  /// Get the underlying route table
  std::shared_ptr<RouteTable> table() const;

  // --- Instance method templates ---

  /// Register a static route from a lambda or function object: void(RequestContext &, http::response<...> &).
  template <typename F>
  void add_static_route(const std::string &path, F &&handler) {
    add_static_route(path, handler::make_api_handler(std::forward<F>(handler)));
  }

  /// Register a static route with HTTP method from a lambda.
  template <typename F>
  void add_static_route(const std::string &path, http::verb method, F &&handler) {
    add_static_route(path, method, handler::make_api_handler(std::forward<F>(handler)));
  }

  /// Register a dynamic route (e.g. "/api/users/{id}"); path parameters are read via ctx.path_parameter(...).
  template <typename F>
  void add_dynamic_route(const std::string &pattern, F &&handler) {
    add_dynamic_route(pattern, handler::make_api_handler(std::forward<F>(handler)));
  }

  /// Register a dynamic route with HTTP method from a lambda.
  template <typename F>
  void add_dynamic_route(const std::string &pattern, http::verb method, F &&handler) {
    add_dynamic_route(pattern, method, handler::make_api_handler(std::forward<F>(handler)));
  }

  // --- Global singleton API (backward compatible) ---

  /// Get the global router instance (singleton)
  static Router &global_router();

  /// Static route registration on global router (backward compatible)
  static void register_static_handler(const std::string &path, std::shared_ptr<ApiHandler> handler);

  /// Dynamic route registration on global router (backward compatible)
  static void register_dynamic_handler(const std::string &pattern, std::shared_ptr<ApiHandler> handler);

  /// Route resolution on global router (backward compatible)
  static std::shared_ptr<ApiHandler> resolve_route(const std::string &path, server::RequestContext &ctx);

  /// Template wrappers for global router (backward compatible)
  template <typename F>
  static void register_static_handler(const std::string &path, F &&handler) {
    global_router().add_static_route(path, std::forward<F>(handler));
  }

  template <typename F>
  static void register_dynamic_handler(const std::string &pattern, F &&handler) {
    global_router().add_dynamic_route(pattern, std::forward<F>(handler));
  }

 private:
  std::shared_ptr<RouteTable> table_;
};

}  // namespace foxhttp::router

// Macros for backward compatibility - use global router
#define REGISTER_STATIC_HANDLER(Path, HandlerClass)                                             \
  namespace {                                                                                   \
  struct HandlerClass##Register {                                                               \
    HandlerClass##Register() {                                                                  \
      foxhttp::router::Router::register_static_handler(Path, std::make_shared<HandlerClass>()); \
    }                                                                                           \
  } HandlerClass##_register;                                                                    \
  }

#define REGISTER_DYNAMIC_HANDLER(Path, HandlerClass)                                             \
  namespace {                                                                                    \
  struct HandlerClass##Register {                                                                \
    HandlerClass##Register() {                                                                   \
      foxhttp::router::Router::register_dynamic_handler(Path, std::make_shared<HandlerClass>()); \
    }                                                                                            \
  } HandlerClass##_register;                                                                     \
  }
