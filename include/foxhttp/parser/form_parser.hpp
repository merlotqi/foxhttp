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
#include <unordered_map>
#include <vector>

namespace foxhttp {

// Forward declarations
class FormField;
class FormParser;

/**
 * @brief Configuration for form parsing
 */
struct FormConfig
{
    std::size_t max_field_size = 1024 * 1024;     // 1MB max field size
    std::size_t max_total_size = 10 * 1024 * 1024;// 10MB max total size
    std::size_t max_fields = 1000;                // Max number of fields
    bool strict_mode = true;                      // Strict RFC compliance
    bool allow_empty_values = true;               // Allow empty field values
    bool support_arrays = true;                   // Support array notation (name[])
    std::string charset = "UTF-8";                // Default charset
};

class FormField
{
public:
    FormField();
    ~FormField();

    // Field properties
    const std::string &name() const;
    bool is_array() const;
    std::size_t size() const;

    // Value access
    const std::string &value() const;
    const std::vector<std::string> &values() const;
    std::string value_at(std::size_t index) const;

    // Validation
    bool is_valid() const;
    std::string validation_error() const;

private:
    friend class FormParser;

    void set_name(const std::string &name);
    void add_value(const std::string &value);
    void set_single_value(const std::string &value);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * @brief Result type for form parsing
 */
using FormData = std::unordered_map<std::string, std::unique_ptr<FormField>>;

/**
 * @brief Enhanced form parser with configuration and validation
 */
class FormParser : public Parser<FormData>
{
public:
    explicit FormParser(const FormConfig &config = FormConfig{});
    ~FormParser();

    // Parser interface
    std::string name() const override;
    std::string content_type() const override;
    bool supports(const http::request<http::string_body> &req) const override;
    FormData parse(const http::request<http::string_body> &req) const override;

    // Configuration
    const FormConfig &config() const;
    void set_config(const FormConfig &config);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};
REGISTER_PARSER(FormData, FormParser);

}// namespace foxhttp