/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <foxhttp/parser/parser.hpp>
#include <string>

namespace foxhttp {

class PlainTextParser : public Parser<std::string>
{
public:
    std::string name() const override;
    std::string content_type() const override;
    bool supports(const http::request<http::string_body> &req) const override;
    std::string parse(const http::request<http::string_body> &req) const override;
};
REGISTER_PARSER(std::string, PlainTextParser)

}// namespace foxhttp
