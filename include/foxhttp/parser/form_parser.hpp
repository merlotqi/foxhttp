#pragma once

#include <foxhttp/config/configs.hpp>
#include <foxhttp/parser/parser.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace foxhttp::parser {

class FormField;
class FormParser;

using FormConfig = config::FormConfig;

namespace detail {
class FormParserImpl;
class FormFieldImpl;
}  // namespace detail

class FormField {
 public:
  FormField();
  ~FormField();

  const std::string &name() const;
  bool is_array() const;
  std::size_t size() const;

  const std::string &value() const;
  const std::vector<std::string> &values() const;
  std::string value_at(std::size_t index) const;

  bool is_valid() const;
  std::string validation_error() const;

 private:
  friend class FormParser;

  void set_name(const std::string &name);
  void add_value(const std::string &value);
  void set_single_value(const std::string &value);

 private:
  friend class detail::FormParserImpl;
  std::unique_ptr<detail::FormFieldImpl> core_;
};
using FormData = std::unordered_map<std::string, std::unique_ptr<FormField>>;

class FormParser : public Parser<FormData> {
 public:
  explicit FormParser(const FormConfig &config = FormConfig{});
  ~FormParser();

  std::string name() const override;
  std::string content_type() const override;
  bool supports(const http::request<http::string_body> &req) const override;
  FormData parse(const http::request<http::string_body> &req) const override;

  const FormConfig &config() const;
  void set_config(const FormConfig &config);

 private:
  std::unique_ptr<detail::FormParserImpl> core_;
};
REGISTER_PARSER(FormData, FormParser);

}  // namespace foxhttp::parser
