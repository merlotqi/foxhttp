#pragma once

#include <foxhttp/foxhttp.hpp>
#include <functional>
#include <string>

namespace foxhttp::examples {

/// Terminal middleware: resolves ``router::`` handlers for ``ctx.path()`` and writes the response.
/// Use it as the **last** middleware on ``server::global_chain()`` (after CORS, body parsers, static files, etc.).
///
/// The async path reports ``middleware_result::stop`` so the pipeline does not continue after dispatch
/// (required for ``session``'s ``execute_async``).
///
/// Priority is ``highest`` so that, after ``middleware_chain`` sorts by ascending priority value,
/// this terminal middleware runs **last** (after CORS, logging, timers, static files, body parsers).
class router_dispatch_middleware : public priority_middleware<middleware_priority::highest> {
 public:
  std::string name() const override { return "RouterDispatch"; }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override {
    (void)next;
    dispatch(ctx, res);
  }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override {
    (void)next;
    try {
      dispatch(ctx, res);
      callback(middleware_result::stop, "");
    } catch (const std::exception &e) {
      callback(middleware_result::error, e.what());
    }
  }

 private:
  static void dispatch(request_context &ctx, http::response<http::string_body> &res) {
    auto h = router::resolve_route(ctx.path(), ctx);
    if (h) {
      h->handle_request(ctx, res);
    } else {
      res.result(http::status::not_found);
      res.set(http::field::content_type, "text/plain");
      res.body() = "Not Found";
    }
  }
};

}  // namespace foxhttp::examples
