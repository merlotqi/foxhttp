#include <benchmark/benchmark.h>

#include <boost/beast/http.hpp>
#include <foxhttp/foxhttp.hpp>
#include <sstream>
#include <string>

namespace http = boost::beast::http;

static std::string make_url_encoded_form(std::size_t fields) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < fields; ++i) {
    if (i) oss << '&';
    oss << "field" << i << "=value" << i;
  }
  return oss.str();
}

static void BM_FormParserUrlEncoded(benchmark::State &state) {
  const auto fields = static_cast<std::size_t>(state.range(0));
  const std::string body = make_url_encoded_form(fields);

  foxhttp::form_parser parser;
  http::request<http::string_body> req;
  req.method(http::verb::post);
  req.set(http::field::content_type, "application/x-www-form-urlencoded");
  req.body() = body;
  req.prepare_payload();

  for (auto _ : state) {
    auto data = parser.parse(req);
    benchmark::DoNotOptimize(data.size());
  }
}
BENCHMARK(BM_FormParserUrlEncoded)->Arg(4)->Arg(32)->Arg(128);

static void BM_FormParserSupports(benchmark::State &state) {
  foxhttp::form_parser parser;
  http::request<http::string_body> req;
  req.set(http::field::content_type, "application/x-www-form-urlencoded");

  for (auto _ : state) {
    bool ok = parser.supports(req);
    benchmark::DoNotOptimize(ok);
  }
}
BENCHMARK(BM_FormParserSupports);

static void BM_PlaintextParserSmall(benchmark::State &state) {
  foxhttp::plain_text_parser parser;
  http::request<http::string_body> req;
  req.set(http::field::content_type, "text/plain");
  req.body() = std::string(4096, 'x');
  req.prepare_payload();

  for (auto _ : state) {
    std::string s = parser.parse(req);
    benchmark::DoNotOptimize(s.size());
  }
}
BENCHMARK(BM_PlaintextParserSmall);

static void BM_PlaintextParserSupports(benchmark::State &state) {
  foxhttp::plain_text_parser parser;
  http::request<http::string_body> req;
  req.set(http::field::content_type, "text/markdown");

  for (auto _ : state) {
    bool ok = parser.supports(req);
    benchmark::DoNotOptimize(ok);
  }
}
BENCHMARK(BM_PlaintextParserSupports);
