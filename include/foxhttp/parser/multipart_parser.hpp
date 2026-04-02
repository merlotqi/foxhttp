#pragma once

#include <foxhttp/config/configs.hpp>
#include <foxhttp/parser/parser.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace foxhttp::parser {

class MultipartField;
class MultipartParser;
class MultipartStreamParser;

using MultipartConfig = config::MultipartConfig;

using progress_callback =
    std::function<void(std::size_t bytes_received, std::size_t total_bytes, const std::string &field_name)>;

using error_callback = std::function<void(const std::string &error, std::size_t position)>;

namespace detail {
class MultipartFieldImpl;
class MultipartStreamParserImpl;
class MultipartParserImpl;
}  // namespace detail

class MultipartField {
  friend class detail::MultipartParserImpl;
  friend class detail::MultipartStreamParserImpl;

 public:
  MultipartField();
  ~MultipartField();

  const std::string &name() const;
  const std::string &filename() const;
  const std::string &content_type() const;
  const std::string &encoding() const;
  std::size_t size() const;

  const std::string &content() const;
  std::string content_as_string() const;
  std::vector<uint8_t> content_as_bytes() const;

  bool is_file() const;
  bool is_text() const;
  bool is_temporary() const;
  const std::string &temp_file_path() const;

  const std::unordered_map<std::string, std::string> &headers() const;
  std::string header(const std::string &name) const;
  bool has_header(const std::string &name) const;

  bool is_valid() const;
  std::string validation_error() const;

 private:
  friend class MultipartParser;
  friend class MultipartStreamParser;

  void set_name(const std::string &name);
  void set_filename(const std::string &filename);
  void set_content_type(const std::string &content_type);
  void set_encoding(const std::string &encoding);
  void add_header(const std::string &name, const std::string &value);
  void set_content(const std::string &content);
  void set_content(std::vector<uint8_t> content);
  void set_temp_file(const std::string &path);
  void cleanup_temp_file();

 private:
  std::unique_ptr<detail::MultipartFieldImpl> core_;
};
using MultipartData = std::vector<std::unique_ptr<MultipartField>>;

class MultipartParser : public Parser<MultipartData> {
 public:
  explicit MultipartParser(const MultipartConfig &config = MultipartConfig{});
  ~MultipartParser();

  std::string name() const override;
  std::string content_type() const override;
  bool supports(const http::request<http::string_body> &req) const override;
  MultipartData parse(const http::request<http::string_body> &req) const override;

  MultipartData parse_with_boundary(const std::string &body, const std::string &boundary) const;
  std::unique_ptr<MultipartStreamParser> create_stream_parser(const std::string &boundary,
                                                                progress_callback progress_cb = nullptr,
                                                                error_callback error_cb = nullptr) const;

  const MultipartConfig &config() const;
  void set_config(const MultipartConfig &config);

  bool validate_field(const MultipartField &field) const;
  std::vector<std::string> validate_all_fields(const MultipartData &data) const;

 private:
  std::unique_ptr<detail::MultipartParserImpl> core_;
};

class MultipartStreamParser {
 public:
  explicit MultipartStreamParser(const std::string &boundary, const MultipartConfig &config = MultipartConfig{},
                                   progress_callback progress_cb = nullptr, error_callback error_cb = nullptr);
  ~MultipartStreamParser();

  bool feed_data(const char *data, std::size_t size);
  bool feed_data(const std::string &data);
  bool is_complete() const;
  MultipartData get_result();

  void set_progress_callback(progress_callback callback);
  void set_error_callback(error_callback callback);

  std::size_t bytes_processed() const;
  std::size_t current_field_size() const;
  const std::string &current_field_name() const;

  void reset();
  void abort();

 private:
  std::unique_ptr<detail::MultipartStreamParserImpl> core_;
};
REGISTER_PARSER(MultipartData, MultipartParser);

}  // namespace foxhttp::parser
