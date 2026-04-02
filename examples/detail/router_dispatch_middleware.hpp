#pragma once

#include <foxhttp/foxhttp.hpp>
#include <functional>
#include <string>

namespace foxhttp::examples {

namespace beast_http = boost::beast::http;

/// Terminal middleware: resolves ``Router::`` handlers for ``ctx.path()`` and writes the response.
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
  std::string name() const override { return "RouterDispatch"; }

  void operator()(middleware::RequestContext &ctx, beast_http::response<beast_http::string_body> &res,
                  std::function<void()> next) override {
    (void)next;
    dispatch(ctx, res);
  }

  void operator()(middleware::RequestContext &ctx, beast_http::response<beast_http::string_body> &res,
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
  static void dispatch(middleware::RequestContext &ctx, beast_http::response<beast_http::string_body> &res) {
    auto h = router::Router::resolve_route(ctx.path(), ctx);
    if (h) {
      h->handle_request(ctx, res);
    } else {
      res.result(beast_http::status::not_found);
      res.set(beast_http::field::content_type, "text/plain");
      res.body() = "Not Found";
    }
  }
};

}  // namespace foxhttp::examples
