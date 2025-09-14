/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <foxhttp/parser/parser.hpp>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace foxhttp {

class JsonParser;

struct JsonConfig
{
    std::size_t max_size = 10 * 1024 * 1024;// 10MB max JSON size
    std::size_t max_depth = 100;            // Max nesting depth
    bool strict_mode = true;                // Strict JSON compliance
    bool allow_comments = false;            // Allow // and /* */ comments
    bool allow_trailing_commas = false;     // Allow trailing commas
    bool allow_duplicate_keys = true;       // Allow duplicate object keys
    bool validate_schema = false;           // Enable schema validation
    nlohmann::json schema;                  // JSON schema for validation
    std::string charset = "UTF-8";          // Default charset
};

/**
 * @brief JSON validation result
 */
struct JsonValidationResult
{
    bool is_valid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    void add_error(const std::string &error)
    {
        is_valid = false;
        errors.push_back(error);
    }

    void add_warning(const std::string &warning)
    {
        warnings.push_back(warning);
    }
};

class JsonParser : public Parser<nlohmann::json>
{
public:
    explicit JsonParser(const JsonConfig &config = JsonConfig{});
    ~JsonParser();

    // Parser interface
    std::string name() const override;
    std::string content_type() const override;
    bool supports(const http::request<http::string_body> &req) const override;
    nlohmann::json parse(const http::request<http::string_body> &req) const override;

    // Configuration
    const JsonConfig &config() const;
    void set_config(const JsonConfig &config);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};
REGISTER_PARSER(nlohmann::json, JsonParser);

}// namespace foxhttp