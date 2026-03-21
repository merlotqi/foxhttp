#pragma once

#include <foxhttp/middleware/middleware.hpp>
#include <functional>
#include <string>

namespace foxhttp {

/// Keys used with @c request_context::set / @c get when the parsed value is move-only (cannot use @c set_parsed_body).
namespace body_parser_context_keys {
inline constexpr const char *form = "foxhttp.body.form";
inline constexpr const char *multipart = "foxhttp.body.multipart";
}  // namespace body_parser_context_keys

/// Parses the request body using registered parsers (JSON, form, multipart, plain text). JSON and plain text are
/// stored via @c set_parsed_body; form and multipart are stored as @c std::shared_ptr under
/// @c body_parser_context_keys::form / @c body_parser_context_keys::multipart. Unknown Content-Types are skipped.
class body_parser_middleware : public middleware {
 public:
  explicit body_parser_middleware(std::string name = "BodyParserMiddleware", bool bad_request_on_parse_error = true);

  std::string name() const override;
  middleware_priority priority() const override;

  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override;

  void set_bad_request_on_parse_error(bool v) { bad_request_on_error_ = v; }
  bool bad_request_on_parse_error() const { return bad_request_on_error_; }

 private:
  /// @return true if pipeline should stop (error response set), false to call next()
  bool handle_body(request_context &ctx, http::response<http::string_body> &res);

  std::string name_;
  bool bad_request_on_error_;
};

}  // namespace foxhttp
