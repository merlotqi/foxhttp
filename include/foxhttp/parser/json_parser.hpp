#pragma once

#include <foxhttp/config/configs.hpp>
#include <foxhttp/parser/parser.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace foxhttp::parser {

class JsonParser;

struct JsonValidationResult {
  bool is_valid = true;
  std::vector<std::string> errors;
  std::vector<std::string> warnings;

  void add_error(const std::string &error) {
    is_valid = false;
    errors.push_back(error);
  }

  void add_warning(const std::string &warning) { warnings.push_back(warning); }
};

namespace detail {
class JsonParserImpl;
}

class JsonParser : public Parser<nlohmann::json> {
 public:
  explicit JsonParser(const config::JsonConfig &config = config::JsonConfig{});
  ~JsonParser();

  std::string name() const override;
  std::string content_type() const override;
  bool supports(const http::request<http::string_body> &req) const override;
  nlohmann::json parse(const http::request<http::string_body> &req) const override;

  const config::JsonConfig &config() const;
  void set_config(const config::JsonConfig &config);

 private:
  friend class detail::JsonParserImpl;
  std::unique_ptr<detail::JsonParserImpl> core_;
};
REGISTER_PARSER(nlohmann::json, JsonParser);

}  // namespace foxhttp::parser
