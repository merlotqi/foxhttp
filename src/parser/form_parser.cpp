/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <cctype>
#include <foxhttp/parser/form_parser.hpp>
#include <sstream>
#include <stdexcept>

namespace foxhttp {

// ============================================================================
// FormField Implementation
// ============================================================================

class FormField::Impl
{
public:
    std::string name_;
    std::vector<std::string> values_;
    bool is_array_;

    Impl() : is_array_(false) {}
};

FormField::FormField() : pimpl_(std::make_unique<Impl>()) {}

FormField::~FormField() = default;

const std::string &FormField::name() const
{
    return pimpl_->name_;
}

bool FormField::is_array() const
{
    return pimpl_->is_array_;
}

std::size_t FormField::size() const
{
    return pimpl_->values_.size();
}

const std::string &FormField::value() const
{
    static const std::string empty;
    return pimpl_->values_.empty() ? empty : pimpl_->values_[0];
}

const std::vector<std::string> &FormField::values() const
{
    return pimpl_->values_;
}

std::string FormField::value_at(std::size_t index) const
{
    if (index >= pimpl_->values_.size())
    {
        return "";
    }
    return pimpl_->values_[index];
}

bool FormField::is_valid() const
{
    return !pimpl_->name_.empty() && validation_error().empty();
}

std::string FormField::validation_error() const
{
    if (pimpl_->name_.empty())
    {
        return "Field name is empty";
    }
    return "";
}

void FormField::set_name(const std::string &name)
{
    pimpl_->name_ = name;
}

void FormField::add_value(const std::string &value)
{
    pimpl_->values_.push_back(value);
    pimpl_->is_array_ = true;
}

void FormField::set_single_value(const std::string &value)
{
    pimpl_->values_.clear();
    pimpl_->values_.push_back(value);
    pimpl_->is_array_ = false;
}

// ============================================================================
// FormParser Implementation
// ============================================================================

class FormParser::Impl
{
public:
    FormConfig config_;

    // Parsing methods
    FormData parse_pairs(const std::string &data) const;
    std::string extract_field_name(const std::string &pair) const;
    std::string extract_field_value(const std::string &pair) const;
    bool is_array_field(const std::string &name) const;
    std::string normalize_field_name(const std::string &name) const;

    // Validation
    bool validate_field_size(const std::string &name, const std::string &value) const;
    bool validate_total_size(const FormData &data) const;
    bool validate_field_count(const FormData &data) const;
    bool validate_field(const FormField &field) const;
    std::vector<std::string> validate_all_fields(const FormData &data) const;

    // Utility methods
    std::string url_decode(const std::string &encoded) const;
    std::string url_encode(const std::string &decoded) const;
};

FormParser::FormParser(const FormConfig &config) : pimpl_(std::make_unique<Impl>())
{
    pimpl_->config_ = config;
}

FormParser::~FormParser() = default;

std::string FormParser::name() const
{
    return "form";
}

std::string FormParser::content_type() const
{
    return "application/x-www-form-urlencoded";
}

bool FormParser::supports(const http::request<http::string_body> &req) const
{
    auto content_type = req[http::field::content_type];
    return content_type.starts_with("application/x-www-form-urlencoded");
}

FormData FormParser::parse(const http::request<http::string_body> &req) const
{
    // Validate total size
    if (req.body().size() > pimpl_->config_.max_total_size)
    {
        throw std::runtime_error("Request body exceeds maximum total size");
    }

    return pimpl_->parse_pairs(req.body());
}

// parse_string moved to private implementation

const FormConfig &FormParser::config() const
{
    return pimpl_->config_;
}

void FormParser::set_config(const FormConfig &config)
{
    pimpl_->config_ = config;
}

// Validation and utility methods moved to private implementation

// ============================================================================
// FormParser::Impl Implementation
// ============================================================================

FormData FormParser::Impl::parse_pairs(const std::string &data) const
{
    FormData result;
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
            it->second->add_value(value);
        }
        else
        {
            // Create new field
            auto field = std::make_unique<FormField>();
            field->set_name(normalized_name);
            if (is_array)
            {
                field->add_value(value);
            }
            else
            {
                field->set_single_value(value);
            }
            result[normalized_name] = std::move(field);
        }
    }

    return result;
}

std::string FormParser::Impl::extract_field_name(const std::string &pair) const
{
    size_t pos = pair.find('=');
    return pos != std::string::npos ? pair.substr(0, pos) : pair;
}

std::string FormParser::Impl::extract_field_value(const std::string &pair) const
{
    size_t pos = pair.find('=');
    return pos != std::string::npos ? pair.substr(pos + 1) : "";
}

bool FormParser::Impl::is_array_field(const std::string &name) const
{
    if (!config_.support_arrays)
    {
        return false;
    }
    return boost::ends_with(name, "[]");
}

std::string FormParser::Impl::normalize_field_name(const std::string &name) const
{
    if (is_array_field(name))
    {
        return name.substr(0, name.length() - 2);
    }
    return name;
}

bool FormParser::Impl::validate_field_size(const std::string &name, const std::string &value) const
{
    return name.size() <= config_.max_field_size && value.size() <= config_.max_field_size;
}

bool FormParser::Impl::validate_total_size(const FormData &data) const
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

bool FormParser::Impl::validate_field_count(const FormData &data) const
{
    return data.size() <= config_.max_fields;
}

bool FormParser::Impl::validate_field(const FormField &field) const
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

std::vector<std::string> FormParser::Impl::validate_all_fields(const FormData &data) const
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

std::string FormParser::Impl::url_decode(const std::string &encoded) const
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

std::string FormParser::Impl::url_encode(const std::string &decoded) const
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

}// namespace foxhttp
