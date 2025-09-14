/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <chrono>
#include <foxhttp/parser/parser.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace foxhttp {

class MultipartField;
class MultipartParser;
class MultipartStreamParser;

struct MultipartConfig
{
    std::size_t max_field_size = 10 * 1024 * 1024; // 10MB max field size
    std::size_t max_file_size = 100 * 1024 * 1024; // 100MB max file size
    std::size_t max_total_size = 500 * 1024 * 1024;// 500MB max total size
    std::size_t memory_threshold = 1024 * 1024;    // 1MB threshold for temp files
    std::string temp_directory = "/tmp";           // Temporary directory
    bool auto_cleanup = true;                      // Auto cleanup temp files
    std::chrono::seconds temp_file_lifetime{3600}; // 1 hour temp file lifetime
    bool strict_mode = true;                       // Strict RFC compliance
    bool allow_empty_fields = false;               // Allow empty field names
    std::vector<std::string> allowed_extensions;   // Allowed file extensions
    std::vector<std::string> allowed_content_types;// Allowed content types
};

using ProgressCallback =
        std::function<void(std::size_t bytes_received, std::size_t total_bytes, const std::string &field_name)>;

using ErrorCallback = std::function<void(const std::string &error, std::size_t position)>;

class MultipartField
{
public:
    MultipartField();
    ~MultipartField();

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
    friend class MultipartParser;
    friend class MultipartStreamParser;

    // Internal methods (used by parser)
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
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};
using MultipartData = std::vector<std::unique_ptr<MultipartField>>;

class MultipartParser : public Parser<MultipartData>
{
public:
    explicit MultipartParser(const MultipartConfig &config = MultipartConfig{});
    ~MultipartParser();

    // Parser interface
    std::string name() const override;
    std::string content_type() const override;
    bool supports(const http::request<http::string_body> &req) const override;
    MultipartData parse(const http::request<http::string_body> &req) const override;

    // Advanced parsing methods
    MultipartData parse_with_boundary(const std::string &body, const std::string &boundary) const;
    std::unique_ptr<MultipartStreamParser> create_stream_parser(const std::string &boundary,
                                                                ProgressCallback progress_cb = nullptr,
                                                                ErrorCallback error_cb = nullptr) const;

    // Configuration
    const MultipartConfig &config() const;
    void set_config(const MultipartConfig &config);

    // Validation
    bool validate_field(const MultipartField &field) const;
    std::vector<std::string> validate_all_fields(const MultipartData &data) const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

class MultipartStreamParser
{
public:
    explicit MultipartStreamParser(const std::string &boundary, const MultipartConfig &config = MultipartConfig{},
                                   ProgressCallback progress_cb = nullptr, ErrorCallback error_cb = nullptr);
    ~MultipartStreamParser();

    // Streaming interface
    bool feed_data(const char *data, std::size_t size);
    bool feed_data(const std::string &data);
    bool is_complete() const;
    MultipartData get_result();

    // Progress and error handling
    void set_progress_callback(ProgressCallback callback);
    void set_error_callback(ErrorCallback callback);

    // State information
    std::size_t bytes_processed() const;
    std::size_t current_field_size() const;
    const std::string &current_field_name() const;

    // Control
    void reset();
    void abort();

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};
REGISTER_PARSER(MultipartData, MultipartParser);

}// namespace foxhttp
