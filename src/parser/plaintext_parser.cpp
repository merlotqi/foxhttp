/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <foxhttp/parser/plaintext_parser.hpp>

namespace foxhttp {

std::string PlainTextParser::name() const
{
    return "plain";
}
std::string PlainTextParser::content_type() const
{
    return "text/plain";
}

bool PlainTextParser::supports(const http::request<http::string_body> &req) const
{
    auto ct = req[http::field::content_type];
    return ct.starts_with("text/plain");
}

std::string PlainTextParser::parse(const http::request<http::string_body> &req) const
{
    return req.body();
}

}// namespace foxhttp
