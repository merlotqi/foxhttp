#include <boost/algorithm/string.hpp>
#include <foxhttp/middleware/basic/body_parser_middleware.hpp>
#include <foxhttp/parser/form_parser.hpp>
#include <foxhttp/parser/json_parser.hpp>
#include <foxhttp/parser/multipart_parser.hpp>
#include <foxhttp/parser/parser.hpp>
#include <foxhttp/parser/plaintext_parser.hpp>
#include <memory>
#include <nlohmann/json.hpp>

namespace foxhttp {

namespace {

std::string content_type_base(const std::string &raw) {
  auto pos = raw.find(';');
  std::string s = raw.substr(0, pos);
  boost::algorithm::trim(s);
  boost::algorithm::to_lower(s);
  return s;
}

}  // namespace

body_parser_middleware::body_parser_middleware(std::string name, bool bad_request_on_parse_error)
    : name_(std::move(name)), bad_request_on_error_(bad_request_on_parse_error) {}

std::string body_parser_middleware::name() const { return name_; }

middleware_priority body_parser_middleware::priority() const { return middleware_priority::normal; }

bool body_parser_middleware::handle_body(request_context &ctx, http::response<http::string_body> &res) {
  const auto &req = ctx.raw_request();
  if (req.body().empty()) {
    return false;
  }

  const std::string ct_raw = ctx.header(http::field::content_type);
  if (ct_raw.empty()) {
    return false;
  }

  const std::string base = content_type_base(ct_raw);

  try {
    if (base == "application/json") {
      auto p = parser_factory::instance().get_best<nlohmann::json>(req);
      if (!p->supports(req)) {
        return false;
      }
      ctx.set_parsed_body(p->parse(req));
      return false;
    }

    if (base == "application/x-www-form-urlencoded") {
      auto p = parser_factory::instance().get_best<form_data>(req);
      if (!p->supports(req)) {
        return false;
      }
      auto data = p->parse(req);
      ctx.set<std::shared_ptr<form_data>>(body_parser_context_keys::form, std::make_shared<form_data>(std::move(data)));
      return false;
    }

    if (boost::algorithm::starts_with(base, "multipart/")) {
      auto p = parser_factory::instance().get_best<multipart_data>(req);
      if (!p->supports(req)) {
        return false;
      }
      auto data = p->parse(req);
      ctx.set<std::shared_ptr<multipart_data>>(body_parser_context_keys::multipart,
                                               std::make_shared<multipart_data>(std::move(data)));
      return false;
    }

    if (boost::algorithm::starts_with(base, "text/")) {
      auto p = parser_factory::instance().get_best<std::string>(req);
      if (!p->supports(req)) {
        return false;
      }
      ctx.set_parsed_body(p->parse(req));
      return false;
    }
  } catch (const std::exception &e) {
    if (bad_request_on_error_) {
      res.result(http::status::bad_request);
      res.set(http::field::content_type, "text/plain; charset=utf-8");
      res.body() = e.what();
      res.prepare_payload();
      return true;
    }
    return false;
  }

  return false;
}

void body_parser_middleware::operator()(request_context &ctx, http::response<http::string_body> &res,
                                        std::function<void()> next) {
  if (handle_body(ctx, res)) {
    return;
  }
  next();
}

void body_parser_middleware::operator()(request_context &ctx, http::response<http::string_body> &res,
                                        std::function<void()> next, async_middleware_callback callback) {
  if (handle_body(ctx, res)) {
    callback(middleware_result::stop, "");
    return;
  }
  next();
  callback(middleware_result::continue_, "");
}

}  // namespace foxhttp
