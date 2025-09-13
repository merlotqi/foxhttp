/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <foxhttp/core/request_context.hpp>

namespace foxhttp {

class Middleware
{
public:
    virtual ~Middleware() = default;
    virtual void operator()(const RequestContext &ctx, http::response<http::string_body> &res,
                            std::function<void()> next) = 0;
};

}// namespace foxhttp