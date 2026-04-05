#pragma once

#include <foxhttp/middleware/middleware.hpp>
#include <functional>
#include <string>

namespace foxhttp::middleware {

/// Keys used with @c RequestContext::set / @c get when the parsed value is move-only (cannot use @c set_parsed_body).
namespace body_parser_context_keys {
inline constexpr const char *form = "foxhttp.body.form";
inline constexpr const char *multipart = "foxhttp.body.multipart";
}  // namespace body_parser_context_keys

/// Parses the request body using registered parsers (JSON, form, multipart, plain text). JSON and plain text are
/// stored via @c set_parsed_body; form and multipart are stored as @c std::shared_ptr under
/// @c body_parser_context_keys::form / @c body_parser_context_keys::multipart. Unknown Content-Types are skipped.
class BodyParserMiddleware : public Middleware {
 public:
  explicit BodyParserMiddleware(std::string name = "BodyParserMiddleware", bool bad_request_on_parse_error = true);

  std::string name() const override;
  MiddlewarePriority priority() const override;

  void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
  void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override;

  void set_bad_request_on_parse_error(bool v) { bad_request_on_error_ = v; }
  bool bad_request_on_parse_error() const { return bad_request_on_error_; }

 private:
  /// @return true if pipeline should stop (error response set), false to call next()
  bool handle_body(RequestContext &ctx, http::response<http::string_body> &res);

  std::string name_;
  bool bad_request_on_error_;
};

}  // namespace foxhttp::middleware
