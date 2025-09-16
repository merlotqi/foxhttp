/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <foxhttp/config/configs.hpp>
#include <foxhttp/parser/parser.hpp>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace foxhttp {

class json_parser;

using json_config = ::foxhttp::json_config;

struct json_validation_result
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

namespace details {
class json_parser_core;
}

class json_parser : public parser<nlohmann::json>
{
public:
    explicit json_parser(const json_config &config = json_config{});
    ~json_parser();

    std::string name() const override;
    std::string content_type() const override;
    bool supports(const http::request<http::string_body> &req) const override;
    nlohmann::json parse(const http::request<http::string_body> &req) const override;

    const json_config &config() const;
    void set_config(const json_config &config);

private:
    friend class details::json_parser_core;
    std::unique_ptr<details::json_parser_core> core_;
};
REGISTER_PARSER(nlohmann::json, json_parser);

}// namespace foxhttp