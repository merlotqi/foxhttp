/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <chrono>
#include <foxhttp/parser/parser.hpp>
#include <foxhttp/config/configs.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace foxhttp {

class multipart_field;
class multipart_parser;
class multipart_stream_parser;

using multipart_config = ::foxhttp::multipart_config;

using progress_callback =
        std::function<void(std::size_t bytes_received, std::size_t total_bytes, const std::string &field_name)>;

using error_callback = std::function<void(const std::string &error, std::size_t position)>;

namespace details
{
class multipart_field_core;
class multipart_stream_parser_core;
class multipart_parser_core;
} // namespace details

class multipart_field
{
    friend class details::multipart_parser_core;
    friend class details::multipart_stream_parser_core;
public:
    multipart_field();
    ~multipart_field();

    // Field properties
    const std::string &name() const;
    const std::string &filename() const;
    const std::string &content_type() const;
    const std::string &encoding() const;
    std::size_t size() const;

    // Content access
    const std::string &content() const;
    std::string content_as_string() const;
    std::vector<uint8_t> content_as_bytes() const;

    // File operations
    bool is_file() const;
    bool is_text() const;
    bool is_temporary() const;
    const std::string &temp_file_path() const;

    // Headers
    const std::unordered_map<std::string, std::string> &headers() const;
    std::string header(const std::string &name) const;
    bool has_header(const std::string &name) const;

    // Validation
    bool is_valid() const;
    std::string validation_error() const;

private:
    friend class multipart_parser;
    friend class multipart_stream_parser;

    // Internal methods (used by parser)
    void _set_name(const std::string &name);
    void _set_filename(const std::string &filename);
    void _set_content_type(const std::string &content_type);
    void _set_encoding(const std::string &encoding);
    void _add_header(const std::string &name, const std::string &value);
    void _set_content(const std::string &content);
    void _set_content(std::vector<uint8_t> content);
    void _set_temp_file(const std::string &path);
    void _cleanup_temp_file();

private:
    std::unique_ptr<details::multipart_field_core> core_;
};
using multipart_data = std::vector<std::unique_ptr<multipart_field>>;

class multipart_parser : public parser<multipart_data>
{
public:
    explicit multipart_parser(const multipart_config &config = multipart_config{});
    ~multipart_parser();

    // Parser interface
    std::string name() const override;
    std::string content_type() const override;
    bool supports(const http::request<http::string_body> &req) const override;
    multipart_data parse(const http::request<http::string_body> &req) const override;

    // Advanced parsing methods
    multipart_data parse_with_boundary(const std::string &body, const std::string &boundary) const;
    std::unique_ptr<multipart_stream_parser> create_stream_parser(const std::string &boundary,
                                                                progress_callback progress_cb = nullptr,
                                                                error_callback error_cb = nullptr) const;

    // Configuration
    const multipart_config &config() const;
    void set_config(const multipart_config &config);

    // Validation
    bool validate_field(const multipart_field &field) const;
    std::vector<std::string> validate_all_fields(const multipart_data &data) const;

private:
    std::unique_ptr<details::multipart_parser_core> core_;
};

class multipart_stream_parser
{
public:
    explicit multipart_stream_parser(const std::string &boundary, const multipart_config &config = multipart_config{},
                                   progress_callback progress_cb = nullptr, error_callback error_cb = nullptr);
    ~multipart_stream_parser();

    // Streaming interface
    bool feed_data(const char *data, std::size_t size);
    bool feed_data(const std::string &data);
    bool is_complete() const;
    multipart_data get_result();

    // Progress and error handling
    void set_progress_callback(progress_callback callback);
    void set_error_callback(error_callback callback);

    // State information
    std::size_t bytes_processed() const;
    std::size_t current_field_size() const;
    const std::string &current_field_name() const;

    // Control
    void reset();
    void abort();

private:
    std::unique_ptr<details::multipart_stream_parser_core> core_;
};
REGISTER_PARSER(multipart_data, multipart_parser);

}// namespace foxhttp
