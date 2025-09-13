/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include "parser.hpp"
#include <nlohmann/json.hpp>

namespace foxhttp {

class JsonParser : public Parser<nlohmann::json>
{
public:
    std::string name() const override;
    std::string content_type() const override;
    bool supports(const http::request<http::string_body> &req) const override;
    nlohmann::json parse(const http::request<http::string_body> &req) const override;
};
REGISTER_PARSER(nlohmann::json, JsonParser);

}// namespace foxhttp