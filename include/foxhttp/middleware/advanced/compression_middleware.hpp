#pragma once

#include <foxhttp/middleware/middleware.hpp>

namespace foxhttp::middleware::advanced {

class CompressionMiddleware : public PriorityMiddleware<MiddlewarePriority::High> {
 public:
  enum class CompressionType {
    gzip,
    deflate,
    br,
    none,
  };

  explicit CompressionMiddleware(size_t threshold = 1024, bool enable_gzip = true, bool enable_deflate = true,
                                  bool enable_br = true);
  std::string name() const override;
  void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
  void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override;

 private:
  bool should_compress(const RequestContext &ctx, const http::response<http::string_body> &res);
  void compress_response(const RequestContext &ctx, http::response<http::string_body> &res);
  std::string get_encoding_name(CompressionType type);
  std::string compress_gzip(const std::string &input);
  std::string compress_deflate(const std::string &input);
  std::string compress_brotli(const std::string &input);

 private:
  size_t threshold_;
  std::vector<CompressionType> supported_types_;
};

}  // namespace foxhttp::middleware::advanced
