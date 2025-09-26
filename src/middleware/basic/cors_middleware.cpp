/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * middleware utilities and helper classes
 */

#include <foxhttp/middleware/basic/cors_middleware.hpp>

namespace foxhttp {

cors_middleware::cors_middleware(const std::string &origin, const std::string &methods, const std::string &headers,
                                 bool allow_credentials, long max_age)
    : origin_(origin), methods_(methods), headers_(headers), allow_credentials_(allow_credentials), max_age_(max_age)
{
}

std::string cors_middleware::name() const
{
    return "CorsMiddleware";
}

void cors_middleware::operator()(request_context &ctx, http::response<http::string_body> &res,
                                 std::function<void()> next)
{
    // Set CORS headers
    if (origin_ == "*")
    {
        res.set("Access-Control-Allow-Origin", ctx.header("Origin"));
    }
    else
    {
        res.set("Access-Control-Allow-Origin", origin_);
    }

    res.set("Access-Control-Allow-Methods", methods_);
    res.set("Access-Control-Allow-Headers", headers_);
    res.set("Access-Control-Max-Age", std::to_string(max_age_));
    if (allow_credentials_)
    {
        res.set("Access-Control-Allow-Credentials", "true");
    }

    // Handle preflight requests
    if (ctx.method() == http::verb::options)
    {
        res.result(http::status::ok);
        res.body() = "";
        res.prepare_payload();
        return;
    }

    next();
}

void cors_middleware::operator()(request_context &ctx, http::response<http::string_body> &res,
                                 std::function<void()> next, async_middleware_callback callback)
{
    // Set CORS headers
    if (origin_ == "*")
    {
        res.set("Access-Control-Allow-Origin", ctx.header("Origin"));
    }
    else
    {
        res.set("Access-Control-Allow-Origin", origin_);
    }

    res.set("Access-Control-Allow-Methods", methods_);
    res.set("Access-Control-Allow-Headers", headers_);
    res.set("Access-Control-Max-Age", std::to_string(max_age_));
    if (allow_credentials_)
    {
        res.set("Access-Control-Allow-Credentials", "true");
    }

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

}// namespace foxhttp
