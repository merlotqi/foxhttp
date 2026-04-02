#pragma once

#include <foxhttp/config/configs.hpp>
#include <string>

namespace foxhttp::parser::detail {

class PlainTextParserImpl {
 public:
  config::PlainTextConfig config_;

  // Processing methods
  std::string process_text(const std::string &text) const;
  bool is_allowed_content_type(const std::string &content_type) const;

  // Utility methods
  std::string normalize_line_endings(const std::string &text) const;
  std::string trim_whitespace(const std::string &text) const;
  bool is_valid_utf8(const std::string &text) const;
};

}  // namespace foxhttp::parser::detail
