#pragma once

#include <foxhttp/handler/callable_api_handler.hpp>
#include <foxhttp/server/request_context.hpp>
#include <memory>
#include <string>
#include <utility>

namespace foxhttp::router {

using ApiHandler = handler::ApiHandler;

class Router {
 public:
  static void register_static_handler(const std::string &path, std::shared_ptr<ApiHandler> handler);
  static void register_dynamic_handler(const std::string &pattern, std::shared_ptr<ApiHandler> handler);

  /// Register a static route from a lambda or function object: void(RequestContext &, http::response<...> &).
  template <typename F>
  static void register_static_handler(const std::string &path, F &&handler) {
    register_static_handler(path, handler::make_api_handler(std::forward<F>(handler)));
  }

  /// Register a dynamic route (e.g. "/api/users/{id}"); path parameters are read via ctx.path_parameter(...).
  template <typename F>
  static void register_dynamic_handler(const std::string &pattern, F &&handler) {
    register_dynamic_handler(pattern, handler::make_api_handler(std::forward<F>(handler)));
  }

  static std::shared_ptr<ApiHandler> resolve_route(const std::string &path, server::RequestContext &ctx);
};

}  // namespace foxhttp::router

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
