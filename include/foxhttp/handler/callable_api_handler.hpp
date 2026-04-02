#pragma once

#include <foxhttp/handler/api_handler.hpp>
#include <foxhttp/server/request_context.hpp>
#include <memory>
#include <type_traits>
#include <utility>

namespace foxhttp::handler {

/// Wraps a callable with signature void(RequestContext &, http::response<http::string_body> &) as ApiHandler.
template <typename F>
class CallableApiHandler final : public ApiHandler {
 public:
  explicit CallableApiHandler(F fn) : fn_(std::move(fn)) {}

  void handle_request(server::RequestContext &ctx, http::response<http::string_body> &res) override {
    fn_(ctx, res);
  }

 private:
  F fn_;
};

/// Type-erasing factory for lambdas and function objects (static and dynamic routes use the same signature).
template <typename F>
std::shared_ptr<ApiHandler> make_api_handler(F &&fn) {
  return std::make_shared<CallableApiHandler<std::decay_t<F>>>(std::forward<F>(fn));
}

}  // namespace foxhttp::handler
