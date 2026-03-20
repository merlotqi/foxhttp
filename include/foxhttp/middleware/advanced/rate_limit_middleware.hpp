#pragma once

#include <boost/beast/http.hpp>
#include <chrono>
#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/server/request_context.hpp>
#include <mutex>
#include <string>
#include <unordered_map>

namespace http = boost::beast::http;

namespace foxhttp {
namespace advanced {

class rate_limit_middleware : public middleware {
 public:
  using key_extractor = std::function<std::string(request_context &)>;

  // Simple sliding window limiter: allow 'max_requests' per 'window'
  explicit rate_limit_middleware(std::size_t max_requests = 60, std::chrono::seconds window = std::chrono::seconds(60))
      : max_requests_(max_requests), window_(window) {}

  rate_limit_middleware &set_key_extractor(key_extractor extractor) {
    extractor_ = std::move(extractor);
    return *this;
  }

  std::string name() const override { return "advanced_rate_limit_middleware"; }
  middleware_priority priority() const override { return middleware_priority::high; }

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override {
    auto key = make_key(ctx);
    if (is_limited(key)) {
      res.result(http::status::too_many_requests);
      res.set(http::field::content_type, "application/json");
      res.set("Retry-After", std::to_string(static_cast<int>(window_.count())));
      res.body() = R"({"error":"rate_limited"})";
      res.prepare_payload();
      return;
    }
    next();
  }

 private:
  struct bucket {
    std::size_t count = 0;
    std::chrono::steady_clock::time_point start;
  };

  bool is_limited(const std::string &key) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto now = std::chrono::steady_clock::now();
    auto &b = buckets_[key];
    if (b.start.time_since_epoch().count() == 0) {
      b.start = now;
      b.count = 1;
      return false;
    }
    if (now - b.start >= window_) {
      b.start = now;
      b.count = 1;
      return false;
    }
    if (b.count >= max_requests_) return true;
    b.count++;
    return false;
  }

  std::string make_key(request_context &ctx) {
    if (extractor_) return extractor_(ctx);
    // Default: X-Forwarded-For + path
    auto ip = ctx.header("X-Forwarded-For", "");
    if (ip.empty()) ip = "unknown";
    return ip + "|" + ctx.path();
  }

 private:
  std::size_t max_requests_;
  std::chrono::seconds window_;
  key_extractor extractor_;
  std::mutex mutex_;
  std::unordered_map<std::string, bucket> buckets_;
};

}  // namespace advanced
}  // namespace foxhttp
