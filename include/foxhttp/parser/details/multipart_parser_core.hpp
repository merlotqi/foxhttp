#pragma once

#include <foxhttp/config/configs.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace foxhttp {

class multipart_field;
class multipart_stream_parser;
using multipart_data = std::vector<std::unique_ptr<multipart_field>>;
using progress_callback = std::function<void(std::size_t, std::size_t, const std::string &)>;
using error_callback = std::function<void(const std::string &, std::size_t)>;

namespace details {

class multipart_field_core {
 public:
  std::string name_;
  std::string filename_;
  std::string content_type_;
  std::string encoding_;
  std::unordered_map<std::string, std::string> headers_;

  mutable std::string content_;
  mutable std::vector<uint8_t> binary_content_;
  mutable std::optional<std::string> temp_file_;
  mutable bool content_loaded_ = false;

  void load_content() const;
  std::string to_lower(const std::string &str) const;
};

class multipart_parser_core {
 public:
  multipart_config config_;

  std::string extract_boundary(const std::string &content_type) const;
  std::unique_ptr<multipart_field> parse_field(const std::string &field_data) const;
  std::unordered_map<std::string, std::string> parse_headers(const std::string &header_data) const;
  std::string decode_content(const std::string &content, const std::string &encoding) const;
  std::vector<uint8_t> decode_binary_content(const std::string &content, const std::string &encoding) const;

  std::string trim(const std::string &str) const;
  std::string to_lower(const std::string &str) const;
  std::string generate_temp_filename() const;
  bool is_allowed_extension(const std::string &filename) const;
  bool is_allowed_content_type(const std::string &content_type) const;

  std::vector<uint8_t> base64_decode(const std::string &encoded) const;
  std::string base64_decode_string(const std::string &encoded) const;
  std::string quoted_printable_decode(const std::string &encoded) const;
};

class multipart_stream_parser_core {
 public:
  enum class ParseState {
    BOUNDARY,
    HEADERS,
    CONTENT,
    COMPLETE,
    error
  };

  multipart_config config_;
  std::string boundary_;
  std::string full_boundary_;
  std::string end_boundary_;

  ParseState state_;
  std::size_t bytes_processed_;
  std::size_t current_field_size_;
  std::string current_field_name_;

  std::string buffer_;
  std::unique_ptr<multipart_field> current_field_;
  multipart_data result_;

  progress_callback progress_callback_;
  error_callback error_callback_;

  bool process_boundary();
  bool process_headers();
  bool process_content();
  void finalize_current_field();
  void handle_error(const std::string &error);
  void write_content_data(const char *data, std::size_t size);
  void switch_to_temp_file();
  std::string generate_temp_filename() const;
  void cleanup_temp_files();
};

}  // namespace details
}  // namespace foxhttp
