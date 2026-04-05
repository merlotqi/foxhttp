#pragma once

#include <foxhttp/core/constants.hpp>
#include <foxhttp/foxhttp.hpp>
#include <functional>
#include <string>

namespace http = boost::beast::http;

namespace foxhttp::examples {

/// Terminal middleware: resolves ``Router::`` handlers for ``ctx.path()`` and writes the response.
///
/// Supports:
/// - Instance-based router via constructor injection
/// - HTTP method matching with automatic 405 Method Not Allowed responses
/// - Backward compatibility with global router singleton
///
/// Use it as the **last** middleware on ``Server::global_chain()`` (after CORS, body parsers, static files, etc.).
///
/// The async path reports ``MiddlewareResult::Stop`` so the pipeline does not continue after dispatch
/// (required for ``session``'s ``execute_async``).
///
/// Priority is ``highest`` so that, after ``MiddlewareChain`` sorts by ascending priority value,
/// this terminal middleware runs **last** (after CORS, logging, timers, static files, body parsers).
class router_dispatch_middleware
    : public middleware::PriorityMiddleware<middleware::MiddlewarePriority::Highest> {
 public:
  /// Create dispatch middleware using the global router (backward compatible)
  router_dispatch_middleware() = default;

  /// Create dispatch middleware with a specific router instance
  explicit router_dispatch_middleware(std::shared_ptr<router::Router> r) : router_(std::move(r)) {}

  std::string name() const override { return "RouterDispatch"; }

  void operator()(middleware::RequestContext &ctx, http::response<http::string_body> &res,
                  std::function<void()> next) override {
    (void)next;
    dispatch(ctx, res);
  }

  void operator()(middleware::RequestContext &ctx, http::response<http::string_body> &res,
                  std::function<void()> next, middleware::async_middleware_callback callback) override {
    (void)next;
    try {
      dispatch(ctx, res);
      callback(middleware::MiddlewareResult::Stop, "");
    } catch (const std::exception &e) {
      callback(middleware::MiddlewareResult::Error, e.what());
    }
  }

 private:
  void dispatch(middleware::RequestContext &ctx, http::response<http::string_body> &res) {
    auto method = ctx.method();

    if (router_) {
      auto [handler, allowed] = router_->resolver_with_method(ctx.path(), method, ctx);
      if (handler) {
        handler->handle_request(ctx, res);
      } else if (!allowed.empty()) {
        set_405_response(res, allowed);
      } else {
        set_404_response(res);
      }
    } else {
      // Backward compatible: use global router (doesn't support 405)
      auto h = router::Router::resolve_route(ctx.path(), ctx);
      if (h) {
        h->handle_request(ctx, res);
      } else {
        set_404_response(res);
      }
    }
  }

  static void set_404_response(http::response<http::string_body> &res) {
    res.result(http::status::not_found);
    res.set(http::field::content_type, "text/plain");
    res.body() = "Not Found";
  }

  static void set_405_response(http::response<http::string_body> &res,
                                const std::vector<http::verb> &allowed) {
    res.result(http::status::method_not_allowed);
    res.set(http::field::content_type, "text/plain");

    // Build Allow header
    std::string allow_header;
    for (size_t i = 0; i < allowed.size(); ++i) {
      if (i > 0) allow_header += ", ";
      allow_header += core::verb_to_string(allowed[i]);
    }
    res.set(http::field::allow, allow_header);
    res.body() = "Method Not Allowed";
  }

  std::shared_ptr<router::Router> router_;
};

}  // namespace foxhttp::examples
