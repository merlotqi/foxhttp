#pragma once

#include <foxhttp/config/configs.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace foxhttp {

struct json_validation_result;

namespace details {

class json_parser_core {
 public:
  json_config config_;

  nlohmann::json parse_with_validation(const std::string &json_string) const;
  json_validation_result validate_size(const std::string &json_string) const;
  json_validation_result validate_depth(const nlohmann::json &json) const;
  json_validation_result validate_schema(const nlohmann::json &json) const;

  std::size_t calculate_depth(const nlohmann::json &json, std::size_t current_depth = 0) const;
  std::string preprocess_json(const std::string &json_string) const;
};

}  // namespace details
}  // namespace foxhttp
