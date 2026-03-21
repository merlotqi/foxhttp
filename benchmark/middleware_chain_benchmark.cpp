#include <benchmark/benchmark.h>
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/middleware/basic/functional_middleware.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/server/request_context.hpp>
#include <memory>
#include <string>

namespace http = boost::beast::http;

static void BM_MiddlewareChainEmptyExecute(benchmark::State &state) {
  boost::asio::io_context ioc;
  foxhttp::middleware_chain chain(ioc);
  chain.enable_statistics(false);

  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target("/");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;

  for (auto _ : state) {
    chain.execute(ctx, res);
    benchmark::DoNotOptimize(res.result_int());
  }
}
BENCHMARK(BM_MiddlewareChainEmptyExecute);

static void BM_MiddlewareChainPassThroughDepth(benchmark::State &state) {
  boost::asio::io_context ioc;
  foxhttp::middleware_chain chain(ioc);
  chain.enable_statistics(false);

  const int n = static_cast<int>(state.range(0));
  for (int i = 0; i < n; ++i) {
    const std::string name = "noop_" + std::to_string(i);
    chain.use(std::make_shared<foxhttp::functional_middleware>(
        name,
        [](foxhttp::request_context &, http::response<http::string_body> &, std::function<void()> next) {
          next();
        }));
  }

  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target("/bench");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;

  for (auto _ : state) {
    chain.execute(ctx, res);
    benchmark::DoNotOptimize(res.result_int());
  }
  chain.clear();
}
BENCHMARK(BM_MiddlewareChainPassThroughDepth)->Arg(1)->Arg(4)->Arg(12)->Arg(32);
