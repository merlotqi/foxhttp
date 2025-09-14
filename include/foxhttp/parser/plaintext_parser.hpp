/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <foxhttp/parser/parser.hpp>
#include <memory>
#include <string>
#include <vector>

namespace foxhttp {

// Forward declarations
class PlainTextParser;

/**
 * @brief Configuration for plain text parsing
 */
struct PlainTextConfig
{
    std::size_t max_size = 10 * 1024 * 1024;       // 10MB max text size
    bool normalize_line_endings = true;            // Convert CRLF to LF
    bool trim_whitespace = false;                  // Trim leading/trailing whitespace
    bool validate_encoding = true;                 // Validate UTF-8 encoding
    std::string charset = "UTF-8";                 // Expected charset
    std::vector<std::string> allowed_content_types;// Allowed content types
    bool strict_mode = false;                      // Strict content type checking
};

/**
 * @brief Enhanced plain text parser with configuration and validation
 */
class PlainTextParser : public Parser<std::string>
{
public:
    explicit PlainTextParser(const PlainTextConfig &config = PlainTextConfig{});
    ~PlainTextParser();

    // Parser interface
    std::string name() const override;
    std::string content_type() const override;
    bool supports(const http::request<http::string_body> &req) const override;
    std::string parse(const http::request<http::string_body> &req) const override;

    // Configuration
    const PlainTextConfig &config() const;
    void set_config(const PlainTextConfig &config);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

// Register the parser
REGISTER_PARSER(std::string, PlainTextParser);

}// namespace foxhttp
