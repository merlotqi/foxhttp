#pragma once

#include <foxhttp/config/configs.hpp>
#include <foxhttp/parser/parser.hpp>
#include <memory>
#include <string>

namespace foxhttp::parser {

class PlainTextParser;
using PlainTextConfig = config::PlainTextConfig;

namespace detail {
class PlainTextParserImpl;
}  // namespace detail

class PlainTextParser : public Parser<std::string> {
 public:
  explicit PlainTextParser(const PlainTextConfig &config = PlainTextConfig{});
  ~PlainTextParser();

  std::string name() const override;
  std::string content_type() const override;
  bool supports(const http::request<http::string_body> &req) const override;
  std::string parse(const http::request<http::string_body> &req) const override;

  const PlainTextConfig &config() const;
  void set_config(const PlainTextConfig &config);

 private:
  std::unique_ptr<detail::PlainTextParserImpl> core_;
};
REGISTER_PARSER(std::string, PlainTextParser);

}  // namespace foxhttp::parser
