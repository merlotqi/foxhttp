#pragma once

#include <foxhttp/config/configs.hpp>
#include <foxhttp/parser/json_parser.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace foxhttp::parser::detail {

class JsonParserImpl {
 public:
  config::JsonConfig config_;

  nlohmann::json parse_with_validation(const std::string &json_string) const;
  JsonValidationResult validate_size(const std::string &json_string) const;
  JsonValidationResult validate_depth(const nlohmann::json &json) const;
  JsonValidationResult validate_schema(const nlohmann::json &json) const;

  std::size_t calculate_depth(const nlohmann::json &json, std::size_t current_depth = 0) const;
  std::string preprocess_json(const std::string &json_string) const;
};

}  // namespace foxhttp::parser::detail
