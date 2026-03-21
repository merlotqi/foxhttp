#include <benchmark/benchmark.h>
#include <boost/beast/http.hpp>
#include <foxhttp/router/router.hpp>
#include <foxhttp/router/route_table.hpp>
#include <foxhttp/server/request_context.hpp>
#include <sstream>
#include <string>

namespace http = boost::beast::http;

namespace {

void reset_routes() { foxhttp::route_table::instance().clear(); }

auto noop_handler() {
  return [](foxhttp::request_context &, http::response<http::string_body> &) {};
}

}  // namespace

static void BM_RouterResolveStatic(benchmark::State &state) {
  reset_routes();
  foxhttp::router::register_static_handler("/api/ping", noop_handler());

  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target("/api/ping");

  for (auto _ : state) {
    foxhttp::request_context ctx(req);
    auto h = foxhttp::router::resolve_route("/api/ping", ctx);
    benchmark::DoNotOptimize(h.get());
  }
  reset_routes();
}
BENCHMARK(BM_RouterResolveStatic);

static void BM_RouterResolveDynamic(benchmark::State &state) {
  reset_routes();
  foxhttp::router::register_dynamic_handler("/api/users/{id}", noop_handler());

  http::request<http::string_body> req;
  req.target("/api/users/12345");

  for (auto _ : state) {
    foxhttp::request_context ctx(req);
    auto h = foxhttp::router::resolve_route("/api/users/12345", ctx);
    benchmark::DoNotOptimize(h.get());
    benchmark::DoNotOptimize(ctx.path_parameter("id"));
  }
  reset_routes();
}
BENCHMARK(BM_RouterResolveDynamic);

/// No handler: static map miss + full dynamic scan.
static void BM_RouterResolveNotFound(benchmark::State &state) {
  reset_routes();
  foxhttp::router::register_static_handler("/api/ping", noop_handler());
  foxhttp::router::register_dynamic_handler("/api/users/{id}", noop_handler());

  http::request<http::string_body> req;
  req.target("/missing/route");

  for (auto _ : state) {
    foxhttp::request_context ctx(req);
    auto h = foxhttp::router::resolve_route("/missing/route", ctx);
    benchmark::DoNotOptimize(h.get());
  }
  reset_routes();
}
BENCHMARK(BM_RouterResolveNotFound);

/// Static table lookup with many registered paths (hit the last-registered key).
static void BM_RouterResolveStaticTableDepth(benchmark::State &state) {
  const int n = static_cast<int>(state.range(0));
  reset_routes();
  for (int i = 0; i < n; ++i) {
    foxhttp::router::register_static_handler("/bm/s/" + std::to_string(i), noop_handler());
  }
  const std::string path = "/bm/s/" + std::to_string(n - 1);
  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target(path);

  for (auto _ : state) {
    foxhttp::request_context ctx(req);
    auto h = foxhttp::router::resolve_route(path, ctx);
    benchmark::DoNotOptimize(h.get());
  }
  reset_routes();
}
BENCHMARK(BM_RouterResolveStaticTableDepth)->Arg(8)->Arg(64)->Arg(512);

/// Dynamic list scan: decoys use one extra static segment so route_table sorts them before the target;
/// path only matches the final pattern after trying every decoy regex.
static void BM_RouterResolveDynamicScanDepth(benchmark::State &state) {
  const int decoys = static_cast<int>(state.range(0));
  reset_routes();
  for (int i = 0; i < decoys; ++i) {
    foxhttp::router::register_dynamic_handler("/bm/nope/" + std::to_string(i) + "/{x}", noop_handler());
  }
  foxhttp::router::register_dynamic_handler("/bm/final/{id}", noop_handler());
  const std::string path = "/bm/final/ok";
  http::request<http::string_body> req;
  req.target(path);

  for (auto _ : state) {
    foxhttp::request_context ctx(req);
    auto h = foxhttp::router::resolve_route(path, ctx);
    benchmark::DoNotOptimize(h.get());
    benchmark::DoNotOptimize(ctx.path_parameter("id"));
  }
  reset_routes();
}
BENCHMARK(BM_RouterResolveDynamicScanDepth)->Arg(0)->Arg(16)->Arg(64)->Arg(128);

/// Static route wins over a dynamic pattern with the same path shape.
static void BM_RouterStaticBeatsDynamic(benchmark::State &state) {
  reset_routes();
  foxhttp::router::register_dynamic_handler("/api/item/{id}", noop_handler());
  foxhttp::router::register_static_handler("/api/item/exact", noop_handler());

  const std::string path = "/api/item/exact";
  http::request<http::string_body> req;
  req.target(path);

  for (auto _ : state) {
    foxhttp::request_context ctx(req);
    auto h = foxhttp::router::resolve_route(path, ctx);
    benchmark::DoNotOptimize(h.get());
  }
  reset_routes();
}
BENCHMARK(BM_RouterStaticBeatsDynamic);

/// request_context parses path, query string, and Cookie header once per construction.
static void BM_RequestContextConstructSimple(benchmark::State &state) {
  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target("/hello/world");

  for (auto _ : state) {
    foxhttp::request_context ctx(req);
    benchmark::DoNotOptimize(ctx.path().data());
  }
}
BENCHMARK(BM_RequestContextConstructSimple);

static void BM_RequestContextConstructQueryAndCookies(benchmark::State &state) {
  std::ostringstream qs;
  for (int i = 0; i < 32; ++i) {
    if (i) qs << '&';
    qs << "k" << i << "=" << i;
  }
  const std::string target = std::string("/search?") + qs.str();
  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target(target);
  req.set(http::field::cookie, "a=1; b=2; session=abc123def456");

  for (auto _ : state) {
    foxhttp::request_context ctx(req);
    benchmark::DoNotOptimize(ctx.query_parameters().size());
    benchmark::DoNotOptimize(ctx.cookies().size());
  }
}
BENCHMARK(BM_RequestContextConstructQueryAndCookies);
