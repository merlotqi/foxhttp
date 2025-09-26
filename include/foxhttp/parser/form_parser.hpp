/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <foxhttp/config/configs.hpp>
#include <foxhttp/parser/parser.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace foxhttp {

class form_field;
class form_parser;

using form_config = ::foxhttp::form_config;

namespace details {
class form_parser_core;
class form_field_core;
}// namespace details

class form_field
{
public:
    form_field();
    ~form_field();

    const std::string &name() const;
    bool is_array() const;
    std::size_t size() const;

    const std::string &value() const;
    const std::vector<std::string> &values() const;
    std::string value_at(std::size_t index) const;

    bool is_valid() const;
    std::string validation_error() const;

private:
    friend class form_parser;

    void _set_name(const std::string &name);
    void _add_value(const std::string &value);
    void _set_single_value(const std::string &value);

private:
    friend class details::form_parser_core;
    std::unique_ptr<details::form_field_core> core_;
};
using form_data = std::unordered_map<std::string, std::unique_ptr<form_field>>;


class form_parser : public parser<form_data>
{
public:
    explicit form_parser(const form_config &config = form_config{});
    ~form_parser();

    std::string name() const override;
    std::string content_type() const override;
    bool supports(const http::request<http::string_body> &req) const override;
    form_data parse(const http::request<http::string_body> &req) const override;

    const form_config &config() const;
    void set_config(const form_config &config);

private:
    std::unique_ptr<details::form_parser_core> core_;
};
REGISTER_PARSER(form_data, form_parser);

}// namespace foxhttp