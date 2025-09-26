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
#include <vector>

namespace foxhttp {

class plain_text_parser;
using plain_text_config = ::foxhttp::plain_text_config;

namespace details {
class plain_text_parser_core;
}// namespace details

class plain_text_parser : public parser<std::string>
{
public:
    explicit plain_text_parser(const plain_text_config &config = plain_text_config{});
    ~plain_text_parser();

    std::string name() const override;
    std::string content_type() const override;
    bool supports(const http::request<http::string_body> &req) const override;
    std::string parse(const http::request<http::string_body> &req) const override;

    const plain_text_config &config() const;
    void set_config(const plain_text_config &config);

private:
    std::unique_ptr<details::plain_text_parser_core> core_;
};
REGISTER_PARSER(std::string, plain_text_parser);

}// namespace foxhttp
