#include <benchmark/benchmark.h>

#include <boost/beast/http.hpp>
#include <foxhttp/parser/json_parser.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace http = boost::beast::http;

static void BM_JsonParserSmallPayload(benchmark::State &state) {
  foxhttp::json_parser parser;
  http::request<http::string_body> req;
  req.set(http::field::content_type, "application/json");
  req.body() = R"({"ok":true,"items":[1,2,3],"name":"foxhttp"})";
  req.prepare_payload();

  for (auto _ : state) {
    auto j = parser.parse(req);
    benchmark::DoNotOptimize(j.size());
  }
}
BENCHMARK(BM_JsonParserSmallPayload);

static void BM_JsonParserSupportsCheck(benchmark::State &state) {
  foxhttp::json_parser parser;
  http::request<http::string_body> req;
  req.set(http::field::content_type, "application/json");
  req.body() = "{}";

  for (auto _ : state) {
    bool ok = parser.supports(req);
    benchmark::DoNotOptimize(ok);
  }
}
BENCHMARK(BM_JsonParserSupportsCheck);

static void BM_JsonParserSupportsWrongType(benchmark::State &state) {
  foxhttp::json_parser parser;
  http::request<http::string_body> req;
  req.set(http::field::content_type, "text/plain");
  req.body() = "not json";

  for (auto _ : state) {
    bool ok = parser.supports(req);
    benchmark::DoNotOptimize(ok);
  }
}
BENCHMARK(BM_JsonParserSupportsWrongType);

/// Larger document: stresses nlohmann::json::parse + copy into return value.
static void BM_JsonParserMediumArray(benchmark::State &state) {
  nlohmann::json arr = nlohmann::json::array();
  const int n = static_cast<int>(state.range(0));
  arr.get_ref<nlohmann::json::array_t &>().reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    arr.push_back(nlohmann::json{{"i", i}, {"tag", "foxhttp"}, {"ok", true}});
  }
  const std::string payload = arr.dump();

  foxhttp::json_parser parser;
  http::request<http::string_body> req;
  req.set(http::field::content_type, "application/json");
  req.body() = payload;
  req.prepare_payload();

  for (auto _ : state) {
    auto j = parser.parse(req);
    benchmark::DoNotOptimize(j.size());
  }
}
BENCHMARK(BM_JsonParserMediumArray)->Arg(100)->Arg(800);

static void BM_JsonParserDeepNested(benchmark::State &state) {
  nlohmann::json cur = nlohmann::json::object();
  for (int d = 0; d < 24; ++d) {
    nlohmann::json next = nlohmann::json::object();
    next["x"] = std::move(cur);
    cur = std::move(next);
  }
  cur["leaf"] = 42;
  const std::string payload = cur.dump();

  foxhttp::json_parser parser;
  http::request<http::string_body> req;
  req.set(http::field::content_type, "application/json");
  req.body() = payload;
  req.prepare_payload();

  for (auto _ : state) {
    auto j = parser.parse(req);
    benchmark::DoNotOptimize(j["x"].is_object());
  }
}
BENCHMARK(BM_JsonParserDeepNested);
