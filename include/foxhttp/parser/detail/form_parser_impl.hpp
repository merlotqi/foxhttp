#pragma once

#include <foxhttp/config/configs.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace foxhttp::parser {
class FormField;
}

namespace foxhttp::parser::detail {

using form_data = std::unordered_map<std::string, std::unique_ptr<FormField>>;

class FormFieldImpl {
 public:
  std::string name_;
  std::vector<std::string> values_;
  bool is_array_ = false;
};

class FormParserImpl {
 public:
  config::FormConfig config_;

  form_data parse_pairs(const std::string &data) const;
  std::string extract_field_name(const std::string &pair) const;
  std::string extract_field_value(const std::string &pair) const;
  bool is_array_field(const std::string &name) const;
  std::string normalize_field_name(const std::string &name) const;

  bool validate_field_size(const std::string &name, const std::string &value) const;
  bool validate_total_size(const form_data &data) const;
  bool validate_field_count(const form_data &data) const;
  bool validate_field(const FormField &field) const;
  std::vector<std::string> validate_all_fields(const form_data &data) const;

  std::string url_decode(const std::string &encoded) const;
  std::string url_encode(const std::string &decoded) const;
};

}  // namespace foxhttp::parser::detail
