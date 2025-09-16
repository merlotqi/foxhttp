/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <foxhttp/config/configs.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace foxhttp {

class form_field;
using form_data = std::unordered_map<std::string, std::unique_ptr<form_field>>;

namespace details {

class form_field_core
{
public:
    std::string name_;
    std::vector<std::string> values_;
    bool is_array_ = false;
};

class form_parser_core
{
public:
    form_config config_;

    form_data parse_pairs(const std::string &data) const;
    std::string extract_field_name(const std::string &pair) const;
    std::string extract_field_value(const std::string &pair) const;
    bool is_array_field(const std::string &name) const;
    std::string normalize_field_name(const std::string &name) const;

    bool validate_field_size(const std::string &name, const std::string &value) const;
    bool validate_total_size(const form_data &data) const;
    bool validate_field_count(const form_data &data) const;
    bool validate_field(const form_field &field) const;
    std::vector<std::string> validate_all_fields(const form_data &data) const;

    std::string url_decode(const std::string &encoded) const;
    std::string url_encode(const std::string &decoded) const;
};

}// namespace details
}// namespace foxhttp
