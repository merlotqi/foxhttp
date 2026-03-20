#pragma once

#include <foxhttp/handler/callable_api_handler.hpp>
#include <memory>
#include <string>
#include <utility>

namespace foxhttp {

class router {
 public:
  static void register_static_handler(const std::string &path, std::shared_ptr<api_handler> handler);
  static void register_dynamic_handler(const std::string &pattern, std::shared_ptr<api_handler> handler);

  /// Register a static route from a lambda or function object: void(request_context &, http::response<...> &).
  template <typename F>
  static void register_static_handler(const std::string &path, F &&handler) {
    register_static_handler(path, make_api_handler(std::forward<F>(handler)));
  }

  /// Register a dynamic route (e.g. "/api/users/{id}"); path parameters are read via ctx.path_parameter(...).
  template <typename F>
  static void register_dynamic_handler(const std::string &pattern, F &&handler) {
    register_dynamic_handler(pattern, make_api_handler(std::forward<F>(handler)));
  }

  static std::shared_ptr<api_handler> resolve_route(const std::string &path, request_context &ctx);
};

}  // namespace foxhttp

#define REGISTER_STATIC_HANDLER(Path, HandlerClass)                                                                \
  namespace {                                                                                                      \
  struct HandlerClass##Register {                                                                                  \
    HandlerClass##Register() { foxhttp::router::register_static_handler(Path, std::make_shared<HandlerClass>()); } \
  } HandlerClass##_register;                                                                                       \
  }

#define REGISTER_DYNAMIC_HANDLER(Path, HandlerClass)                                                                \
  namespace {                                                                                                       \
  struct HandlerClass##Register {                                                                                   \
    HandlerClass##Register() { foxhttp::router::register_dynamic_handler(Path, std::make_shared<HandlerClass>()); } \
  } HandlerClass##_register;                                                                                        \
  }
