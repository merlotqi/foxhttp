#pragma once

#include <foxhttp/handler/api_handler.hpp>
#include <foxhttp/server/request_context.hpp>
#include <memory>
#include <type_traits>
#include <utility>

namespace foxhttp {

/// Wraps a callable with signature void(request_context &, http::response<http::string_body> &) as api_handler.
template <typename F>
class callable_api_handler final : public api_handler {
 public:
  explicit callable_api_handler(F fn) : fn_(std::move(fn)) {}

  void handle_request(request_context &ctx, http::response<http::string_body> &res) override { fn_(ctx, res); }

 private:
  F fn_;
};

/// Type-erasing factory for lambdas and function objects (static and dynamic routes use the same signature).
template <typename F>
std::shared_ptr<api_handler> make_api_handler(F &&fn) {
  return std::make_shared<callable_api_handler<std::decay_t<F>>>(std::forward<F>(fn));
}

}  // namespace foxhttp
