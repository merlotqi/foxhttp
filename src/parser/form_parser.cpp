/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <foxhttp/parser/form_parser.hpp>

namespace foxhttp {

static std::string url_decode(const std::string &in)
{
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i)
    {
        if (in[i] == '%' && i + 2 < in.size())
        {
            unsigned int val{};
            std::istringstream is(in.substr(i + 1, 2));
            if (is >> std::hex >> val)
            {
                out.push_back(static_cast<char>(val));
                i += 2;
            }
        }
        else if (in[i] == '+')
        {
            out.push_back(' ');
        }
        else
        {
            out.push_back(in[i]);
        }
    }
    return out;
}

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
    return req[http::field::content_type].starts_with("application/x-www-form-urlencoded");
}

std::unordered_map<std::string, std::string> FormParser::parse(const http::request<http::string_body> &req) const
{
    std::unordered_map<std::string, std::string> kv;
    std::istringstream ss(req.body());
    std::string pair;
    while (std::getline(ss, pair, '&'))
    {
        auto pos = pair.find('=');
        if (pos != std::string::npos)
        {
            kv[url_decode(pair.substr(0, pos))] = url_decode(pair.substr(pos + 1));
        }
    }
    return kv;
}
}// namespace foxhttp
