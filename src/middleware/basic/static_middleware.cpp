#include <boost/algorithm/string.hpp>
#include <foxhttp/middleware/basic/static_middleware.hpp>
#include <fstream>
#include <sstream>

namespace foxhttp::middleware {

namespace {

std::unordered_map<std::string, std::string> make_mime_map() {
  return {
      {"html", "text/html; charset=utf-8"},
      {"htm", "text/html; charset=utf-8"},
      {"css", "text/css; charset=utf-8"},
      {"js", "application/javascript; charset=utf-8"},
      {"mjs", "application/javascript; charset=utf-8"},
      {"json", "application/json; charset=utf-8"},
      {"xml", "application/xml; charset=utf-8"},
      {"txt", "text/plain; charset=utf-8"},
      {"text", "text/plain; charset=utf-8"},
      {"md", "text/markdown; charset=utf-8"},
      {"svg", "image/svg+xml"},
      {"png", "image/png"},
      {"jpg", "image/jpeg"},
      {"jpeg", "image/jpeg"},
      {"gif", "image/gif"},
      {"webp", "image/webp"},
      {"ico", "image/x-icon"},
      {"woff", "font/woff"},
      {"woff2", "font/woff2"},
      {"ttf", "font/ttf"},
      {"otf", "font/otf"},
      {"pdf", "application/pdf"},
      {"zip", "application/zip"},
      {"wasm", "application/wasm"},
      {"mp4", "video/mp4"},
      {"webm", "video/webm"},
      {"mp3", "audio/mpeg"},
      {"wav", "audio/wav"},
  };
}

}  // namespace

const std::unordered_map<std::string, std::string> &StaticMiddleware::mime_map() {
  static const auto m = make_mime_map();
  return m;
}

std::string StaticMiddleware::normalize_prefix(std::string p) {
  if (p.empty()) {
    p = "/";
  }
  if (p.front() != '/') {
    p.insert(p.begin(), '/');
  }
  while (p.size() > 1 && p.back() == '/') {
    p.pop_back();
  }
  return p;
}

bool StaticMiddleware::is_subpath(const std::filesystem::path &root, const std::filesystem::path &candidate) {
  std::error_code ec;
  auto r = std::filesystem::weakly_canonical(root, ec);
  if (ec) {
    return false;
  }
  auto c = std::filesystem::weakly_canonical(candidate, ec);
  if (ec) {
    return false;
  }
  auto rit = r.begin();
  auto cit = c.begin();
  for (; rit != r.end(); ++rit, ++cit) {
    if (cit == c.end() || *rit != *cit) {
      return false;
    }
  }
  return true;
}

std::string StaticMiddleware::mime_for_path(const std::filesystem::path &file) {
  auto ext = file.extension().string();
  if (ext.size() > 1 && ext.front() == '.') {
    ext.erase(ext.begin());
  }
  boost::algorithm::to_lower(ext);
  const auto &m = mime_map();
  auto it = m.find(ext);
  if (it != m.end()) {
    return it->second;
  }
  return "application/octet-stream";
}

StaticMiddleware::StaticMiddleware(std::string url_prefix, std::filesystem::path document_root,
                                     std::string index_file)
    : prefix_(normalize_prefix(std::move(url_prefix))),
      doc_root_(std::move(document_root)),
      index_file_(std::move(index_file)),
      max_file_size_(64u * 1024u * 1024u) {
  std::error_code ec;
  auto canon = std::filesystem::weakly_canonical(doc_root_, ec);
  if (!ec) {
    doc_root_ = std::move(canon);
  }
}

std::string StaticMiddleware::name() const { return "StaticMiddleware"; }

MiddlewarePriority StaticMiddleware::priority() const { return MiddlewarePriority::Normal; }

void StaticMiddleware::set_max_file_size(std::size_t bytes) { max_file_size_ = bytes; }

StaticMiddleware::ServeResult StaticMiddleware::try_serve(RequestContext &ctx,
                                                             http::response<http::string_body> &res) {
  const auto verb = ctx.method();
  if (verb != http::verb::get && verb != http::verb::head) {
    return ServeResult::NotApplicable;
  }

  const std::string &path = ctx.path();
  if (path.size() < prefix_.size() || path.compare(0, prefix_.size(), prefix_) != 0) {
    return ServeResult::NotApplicable;
  }

  std::string rel = path.substr(prefix_.size());
  if (rel.empty() || rel.front() != '/') {
    rel.insert(rel.begin(), '/');
  }
  if (rel.size() > 1 && rel.back() == '/') {
    rel.pop_back();
  }

  std::filesystem::path mapped = doc_root_;
  if (rel != "/") {
    mapped /= rel.substr(1);
  }

  std::error_code ec;
  mapped = std::filesystem::weakly_canonical(mapped, ec);
  if (ec || !is_subpath(doc_root_, mapped)) {
    res.result(http::status::forbidden);
    res.set(http::field::content_type, "text/plain; charset=utf-8");
    res.body() = "Forbidden";
    return ServeResult::StopPipeline;
  }

  auto st = std::filesystem::status(mapped, ec);
  if (ec) {
    res.result(http::status::not_found);
    res.set(http::field::content_type, "text/plain; charset=utf-8");
    res.body() = "Not Found";
    return ServeResult::StopPipeline;
  }

  if (std::filesystem::is_directory(st)) {
    mapped /= index_file_;
    mapped = std::filesystem::weakly_canonical(mapped, ec);
    if (ec || !is_subpath(doc_root_, mapped)) {
      res.result(http::status::forbidden);
      res.set(http::field::content_type, "text/plain; charset=utf-8");
      res.body() = "Forbidden";
      return ServeResult::StopPipeline;
    }
    st = std::filesystem::status(mapped, ec);
  }

  if (!std::filesystem::is_regular_file(st)) {
    res.result(http::status::not_found);
    res.set(http::field::content_type, "text/plain; charset=utf-8");
    res.body() = "Not Found";
    return ServeResult::StopPipeline;
  }

  const auto fsize = std::filesystem::file_size(mapped, ec);
  if (ec) {
    res.result(http::status::internal_server_error);
    res.set(http::field::content_type, "text/plain; charset=utf-8");
    res.body() = "Internal Server Error";
    return ServeResult::StopPipeline;
  }

  if (fsize > max_file_size_) {
    res.result(http::status::payload_too_large);
    res.set(http::field::content_type, "text/plain; charset=utf-8");
    res.body() = "Payload Too Large";
    return ServeResult::StopPipeline;
  }

  const std::string mime = mime_for_path(mapped);
  res.result(http::status::ok);
  res.set(http::field::content_type, mime);
  res.keep_alive(ctx.raw_request().keep_alive());

  if (verb == http::verb::head) {
    res.content_length(static_cast<std::size_t>(fsize));
    res.body().clear();
    return ServeResult::Served;
  }

  std::ifstream in(mapped, std::ios::binary);
  if (!in) {
    res.result(http::status::internal_server_error);
    res.set(http::field::content_type, "text/plain; charset=utf-8");
    res.body() = "Failed to read file";
    return ServeResult::StopPipeline;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  res.body() = ss.str();
  return ServeResult::Served;
}

void StaticMiddleware::operator()(RequestContext &ctx, http::response<http::string_body> &res,
                                   std::function<void()> next) {
  const auto r = try_serve(ctx, res);
  if (r == ServeResult::NotApplicable) {
    next();
    return;
  }
  res.prepare_payload();
}

void StaticMiddleware::operator()(RequestContext &ctx, http::response<http::string_body> &res,
                                   std::function<void()> next, async_middleware_callback callback) {
  const auto r = try_serve(ctx, res);
  if (r == ServeResult::NotApplicable) {
    next();
    callback(MiddlewareResult::Continue, "");
    return;
  }
  res.prepare_payload();
  callback(MiddlewareResult::Stop, "");
}

}  // namespace foxhttp::middleware
