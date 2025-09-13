/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <foxhttp/parser/json_parser.hpp>

namespace foxhttp {

std::string JsonParser::name() const
{
    return "json";
}
std::string JsonParser::content_type() const
{
    return "application/json";
}

bool JsonParser::supports(const http::request<http::string_body> &req) const
{
    return req[http::field::content_type] == content_type();
}

nlohmann::json JsonParser::parse(const http::request<http::string_body> &req) const
{
    return nlohmann::json::parse(req.body());
}

}// namespace foxhttp