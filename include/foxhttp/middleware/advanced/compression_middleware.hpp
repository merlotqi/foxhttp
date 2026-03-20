#pragma once

#include <foxhttp/middleware/middleware.hpp>

namespace foxhttp {

class compression_middleware : public priority_middleware<middleware_priority::high> {
 public:
  enum class compression_type {
    gzip,
    deflate,
    br,
    none,
  };

  explicit compression_middleware(size_t threshold = 1024, bool enable_gzip = true, bool enable_deflate = true,
                                  bool enable_br = true);
  std::string name() const override;
  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override;

 private:
  bool should_compress(const request_context &ctx, const http::response<http::string_body> &res);
  void compress_response(const request_context &ctx, http::response<http::string_body> &res);
  std::string get_encoding_name(compression_type type);
  std::string compress_gzip(const std::string &input);
  std::string compress_deflate(const std::string &input);
  std::string compress_brotli(const std::string &input);

 private:
  size_t threshold_;
  std::vector<compression_type> supported_types_;
};

}  // namespace foxhttp
