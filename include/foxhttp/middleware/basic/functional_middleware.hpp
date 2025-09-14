/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * Middleware utilities and helper classes
 */

#pragma once

#include <boost/algorithm/string.hpp>
#include <foxhttp/middleware/basic/cors_middleware.hpp>
#include <foxhttp/middleware/basic/logger_middleware.hpp>
#include <foxhttp/middleware/basic/response_time_middleware.hpp>
#include <foxhttp/middleware/middleware.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace foxhttp {

class FunctionalMiddleware : public Middleware
{
public:
    using SyncFunc = std::function<void(RequestContext &, http::response<http::string_body> &, std::function<void()>)>;
    using AsyncFunc = std::function<void(RequestContext &, http::response<http::string_body> &, std::function<void()>,
                                         AsyncMiddlewareCallback)>;
    using ConditionFunc = std::function<bool(RequestContext &)>;

    FunctionalMiddleware(const std::string &name, SyncFunc sync_func, AsyncFunc async_func = nullptr,
                         ConditionFunc condition = nullptr, middleware_priority priority = middleware_priority::normal,
                         std::chrono::milliseconds timeout = std::chrono::milliseconds{0});

    std::string name() const override;
    middleware_priority priority() const override;
    bool should_execute(RequestContext &ctx) const override;
    std::chrono::milliseconds timeout() const override;

    void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
    void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next,
                    AsyncMiddlewareCallback callback) override;

private:
    std::string name_;
    SyncFunc sync_func_;
    AsyncFunc async_func_;
    ConditionFunc condition_;
    middleware_priority priority_;
    std::chrono::milliseconds timeout_;
};

}// namespace foxhttp
