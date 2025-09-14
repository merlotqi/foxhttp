/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * Middleware utilities and helper classes
 */

#include <foxhttp/middleware/basic/functional_middleware.hpp>

namespace foxhttp {

/* -------------------------- FunctionalMiddleware -------------------------- */

FunctionalMiddleware::FunctionalMiddleware(const std::string &name, SyncFunc sync_func, AsyncFunc async_func,
                                           ConditionFunc condition, middleware_priority priority,
                                           std::chrono::milliseconds timeout)
    : name_(name), sync_func_(std::move(sync_func)), async_func_(std::move(async_func)),
      condition_(std::move(condition)), priority_(priority), timeout_(timeout)
{
}

std::string FunctionalMiddleware::name() const
{
    return name_;
}
middleware_priority FunctionalMiddleware::priority() const
{
    return priority_;
}
bool FunctionalMiddleware::should_execute(RequestContext &ctx) const
{
    return condition_ ? condition_(ctx) : true;
}
std::chrono::milliseconds FunctionalMiddleware::timeout() const
{
    return timeout_;
}

void FunctionalMiddleware::operator()(RequestContext &ctx, http::response<http::string_body> &res,
                                      std::function<void()> next)
{
    sync_func_(ctx, res, next);
}

void FunctionalMiddleware::operator()(RequestContext &ctx, http::response<http::string_body> &res,
                                      std::function<void()> next, AsyncMiddlewareCallback callback)
{
    if (async_func_)
    {
        async_func_(ctx, res, next, callback);
    }
    else
    {
        // Fallback to sync implementation
        Middleware::operator()(ctx, res, next, callback);
    }
}

}// namespace foxhttp