/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace foxhttp {

class RequestContext;
class APIHandler
{
public:
    virtual ~APIHandler() = default;
    virtual void handle_request(RequestContext &ctx, http::response<http::string_body> &res) = 0;
};

}// namespace foxhttp