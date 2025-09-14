/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <chrono>
#include <foxhttp/core/request_context.hpp>
#include <foxhttp/middleware/middleware.hpp>

namespace foxhttp {

class ResponseTimeMiddleware : public PriorityMiddleware<middleware_priority::low>
{
public:
    explicit ResponseTimeMiddleware(const std::string &header_name = "X-Response-Time") : header_name_(header_name) {}

    std::string name() const override
    {
        return "ResponseTimeMiddleware";
    }

    void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next) override
    {
        auto start = std::chrono::steady_clock::now();

        next();

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        res.set(header_name_, std::to_string(duration.count()) + "ms");
    }

    void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next,
                    AsyncMiddlewareCallback callback) override
    {
        auto start = std::chrono::steady_clock::now();

        // Store start time in context for later use
        ctx.set("response_time_start", start);

        next();
        callback(middleware_result::continue_, "");

        // Note: In async mode, we can't easily measure total time here
        // The response time would need to be calculated in the completion callback
    }

private:
    std::string header_name_;
};

}// namespace foxhttp
