#pragma once

#include <filesystem>
#include <foxhttp/middleware/middleware.hpp>
#include <functional>
#include <string>
#include <unordered_map>

namespace foxhttp::middleware {

/// Serves files under @p document_root for HTTP GET/HEAD requests whose path starts with @p url_prefix.
/// Path traversal outside @p document_root is rejected with 403. Missing files under the prefix: 404.
class StaticMiddleware : public Middleware {
 public:
  explicit StaticMiddleware(std::string url_prefix, std::filesystem::path document_root,
                             std::string index_file = "index.html");

  std::string name() const override;
  MiddlewarePriority priority() const override;

  void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
  void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override;

  void set_max_file_size(std::size_t bytes);
  std::size_t max_file_size() const { return max_file_size_; }

 private:
  enum class ServeResult {
    NotApplicable,
    Served,
    StopPipeline
  };

  ServeResult try_serve(RequestContext &ctx, http::response<http::string_body> &res);

  static std::string normalize_prefix(std::string p);
  static bool is_subpath(const std::filesystem::path &root, const std::filesystem::path &candidate);
  static std::string mime_for_path(const std::filesystem::path &file);
  static const std::unordered_map<std::string, std::string> &mime_map();

  std::string prefix_;
  std::filesystem::path doc_root_;
  std::string index_file_;
  std::size_t max_file_size_;
};

}  // namespace foxhttp::middleware
