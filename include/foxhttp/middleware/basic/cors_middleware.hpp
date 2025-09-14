/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <foxhttp/middleware/middleware.hpp>
#include <string>

namespace foxhttp {

class CorsMiddleware : public PriorityMiddleware<middleware_priority::high>
{
public:
    explicit CorsMiddleware(const std::string &origin = "*",
                            const std::string &methods = "GET, POST, PUT, DELETE, OPTIONS",
                            const std::string &headers = "Content-Type, Authorization")
        : origin_(origin), methods_(methods), headers_(headers)
    {
    }

    std::string name() const override
    {
        return "CorsMiddleware";
    }

    void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next) override
    {
        // Set CORS headers
        res.set("Access-Control-Allow-Origin", origin_);
        res.set("Access-Control-Allow-Methods", methods_);
        res.set("Access-Control-Allow-Headers", headers_);
        res.set("Access-Control-Max-Age", "86400");// 24 hours

        // Handle preflight requests
        if (ctx.method() == http::verb::options)
        {
            res.result(http::status::ok);
            return;
        }

        next();
    }

    void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next,
                    AsyncMiddlewareCallback callback) override
    {
        // Set CORS headers
        res.set("Access-Control-Allow-Origin", origin_);
        res.set("Access-Control-Allow-Methods", methods_);
        res.set("Access-Control-Allow-Headers", headers_);
        res.set("Access-Control-Max-Age", "86400");

        // Handle preflight requests
        if (ctx.method() == http::verb::options)
        {
            res.result(http::status::ok);
            callback(middleware_result::stop, "");
            return;
        }

        next();
        callback(middleware_result::continue_, "");
    }

private:
    std::string origin_;
    std::string methods_;
    std::string headers_;
};

}// namespace foxhttp
