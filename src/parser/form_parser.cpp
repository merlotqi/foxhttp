/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <cctype>
#include <foxhttp/parser/details/form_parser_core.hpp>
#include <foxhttp/parser/form_parser.hpp>
#include <sstream>
#include <stdexcept>

namespace foxhttp {

/* ------------------------------- form_field ------------------------------- */

form_field::form_field() : core_(std::make_unique<details::form_field_core>()) {}

form_field::~form_field() = default;

const std::string &form_field::name() const
{
    return core_->name_;
}

bool form_field::is_array() const
{
    return core_->is_array_;
}

std::size_t form_field::size() const
{
    return core_->values_.size();
}

const std::string &form_field::value() const
{
    static const std::string empty;
    return core_->values_.empty() ? empty : core_->values_[0];
}

const std::vector<std::string> &form_field::values() const
{
    return core_->values_;
}

std::string form_field::value_at(std::size_t index) const
{
    if (index >= core_->values_.size())
    {
        return "";
    }
    return core_->values_[index];
}

bool form_field::is_valid() const
{
    return !core_->name_.empty() && validation_error().empty();
}

std::string form_field::validation_error() const
{
    if (core_->name_.empty())
    {
        return "Field name is empty";
    }
    return "";
}

void form_field::_set_name(const std::string &name)
{
    core_->name_ = name;
}

void form_field::_add_value(const std::string &value)
{
    core_->values_.push_back(value);
    core_->is_array_ = true;
}

void form_field::_set_single_value(const std::string &value)
{
    core_->values_.clear();
    core_->values_.push_back(value);
    core_->is_array_ = false;
}

/* ------------------------------- form_parser ------------------------------ */

form_parser::form_parser(const form_config &config) : core_(std::make_unique<details::form_parser_core>())
{
    core_->config_ = config;
}

form_parser::~form_parser() = default;

std::string form_parser::name() const
{
    return "form";
}

std::string form_parser::content_type() const
{
    return "application/x-www-form-urlencoded";
}

bool form_parser::supports(const http::request<http::string_body> &req) const
{
    auto content_type = req[http::field::content_type];
    return content_type.starts_with("application/x-www-form-urlencoded");
}

form_data form_parser::parse(const http::request<http::string_body> &req) const
{
    // Validate total size
    if (req.body().size() > core_->config_.max_total_size)
    {
        throw std::runtime_error("Request body exceeds maximum total size");
    }

    return core_->parse_pairs(req.body());
}

// parse_string moved to private implementation

const form_config &form_parser::config() const
{
    return core_->config_;
}

void form_parser::set_config(const form_config &config)
{
    core_->config_ = config;
}

/* ------------------------ form_parser_core ----------------------- */

namespace details {

form_data form_parser_core::parse_pairs(const std::string &data) const
{
    form_data result;
    std::istringstream ss(data);
    std::string pair;

    while (std::getline(ss, pair, '&'))
    {
        if (pair.empty())
            continue;

        std::string name = extract_field_name(pair);
        std::string value = extract_field_value(pair);

        // Validate field size
        if (!validate_field_size(name, value))
        {
            throw std::runtime_error("Field size exceeds maximum allowed size");
        }

        // Decode name and value
        name = url_decode(name);
        value = url_decode(value);

        // Handle array notation
        std::string normalized_name = normalize_field_name(name);
        bool is_array = is_array_field(name);

        auto it = result.find(normalized_name);
        if (it != result.end())
        {
            // Field already exists, add to array
            it->second->_add_value(value);
        }
        else
        {
            // Create new field
            auto field = std::make_unique<form_field>();
            field->_set_name(normalized_name);
            if (is_array)
            {
                field->_add_value(value);
            }
            else
            {
                field->_set_single_value(value);
            }
            result[normalized_name] = std::move(field);
        }
    }

    return result;
}

std::string form_parser_core::extract_field_name(const std::string &pair) const
{
    size_t pos = pair.find('=');
    return pos != std::string::npos ? pair.substr(0, pos) : pair;
}

std::string form_parser_core::extract_field_value(const std::string &pair) const
{
    size_t pos = pair.find('=');
    return pos != std::string::npos ? pair.substr(pos + 1) : "";
}

bool form_parser_core::is_array_field(const std::string &name) const
{
    if (!config_.support_arrays)
    {
        return false;
    }
    return boost::ends_with(name, "[]");
}

std::string form_parser_core::normalize_field_name(const std::string &name) const
{
    if (is_array_field(name))
    {
        return name.substr(0, name.length() - 2);
    }
    return name;
}

bool form_parser_core::validate_field_size(const std::string &name, const std::string &value) const
{
    return name.size() <= config_.max_field_size && value.size() <= config_.max_field_size;
}

bool form_parser_core::validate_total_size(const form_data &data) const
{
    std::size_t total_size = 0;
    for (const auto &[name, field]: data)
    {
        total_size += name.size();
        for (const auto &value: field->values())
        {
            total_size += value.size();
        }
    }
    return total_size <= config_.max_total_size;
}

bool form_parser_core::validate_field_count(const form_data &data) const
{
    return data.size() <= config_.max_fields;
}

bool form_parser_core::validate_field(const form_field &field) const
{
    if (!field.is_valid())
    {
        return false;
    }

    // Check field size
    for (const auto &value: field.values())
    {
        if (value.size() > config_.max_field_size)
        {
            return false;
        }
    }

    return true;
}

std::vector<std::string> form_parser_core::validate_all_fields(const form_data &data) const
{
    std::vector<std::string> errors;

    // Check field count
    if (!validate_field_count(data))
    {
        errors.push_back("Too many fields");
    }

    // Check total size
    if (!validate_total_size(data))
    {
        errors.push_back("Total data size exceeds limit");
    }

    // Check individual fields
    for (const auto &[name, field]: data)
    {
        if (!validate_field(*field))
        {
            errors.push_back("Field '" + name + "' validation failed");
        }
    }

    return errors;
}

std::string form_parser_core::url_decode(const std::string &encoded) const
{
    std::string decoded;
    decoded.reserve(encoded.size());

    for (size_t i = 0; i < encoded.size(); ++i)
    {
        if (encoded[i] == '%' && i + 2 < encoded.size())
        {
            unsigned int val{};
            std::istringstream is(encoded.substr(i + 1, 2));
            if (is >> std::hex >> val)
            {
                decoded.push_back(static_cast<char>(val));
                i += 2;
            }
            else
            {
                decoded.push_back(encoded[i]);
            }
        }
        else if (encoded[i] == '+')
        {
            decoded.push_back(' ');
        }
        else
        {
            decoded.push_back(encoded[i]);
        }
    }

    return decoded;
}

std::string form_parser_core::url_encode(const std::string &decoded) const
{
    std::string encoded;
    encoded.reserve(decoded.size() * 3);

    for (unsigned char c: decoded)
    {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            encoded.push_back(c);
        }
        else
        {
            encoded.push_back('%');
            encoded.push_back("0123456789ABCDEF"[c >> 4]);
            encoded.push_back("0123456789ABCDEF"[c & 15]);
        }
    }

    return encoded;
}
}// namespace details


}// namespace foxhttp
