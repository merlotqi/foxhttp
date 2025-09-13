/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <foxhttp/parser/parser.hpp>
#include <string>
#include <unordered_map>

namespace foxhttp {

class FormParser : public Parser<std::unordered_map<std::string, std::string>>
{
public:
    std::string name() const override;
    std::string content_type() const override;
    bool supports(const http::request<http::string_body> &req) const override;
    std::unordered_map<std::string, std::string> parse(const http::request<http::string_body> &req) const override;
};
using FormType = std::unordered_map<std::string, std::string>;
REGISTER_PARSER(FormType, FormParser);

}// namespace foxhttp