/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * middleware utilities and helper classes
 */

#include <foxhttp/middleware/basic/functional_middleware.hpp>

namespace foxhttp {

/* -------------------------- functional_middleware -------------------------- */

functional_middleware::functional_middleware(const std::string &name, sync_func sync_func, async_func async_func,
                                             condition_func condition, middleware_priority priority,
                                             std::chrono::milliseconds timeout)
    : name_(name), sync_func_(std::move(sync_func)), async_func_(std::move(async_func)),
      condition_(std::move(condition)), priority_(priority), timeout_(timeout)
{
}

std::string functional_middleware::name() const
{
    return name_;
}
middleware_priority functional_middleware::priority() const
{
    return priority_;
}
bool functional_middleware::should_execute(request_context &ctx) const
{
    return condition_ ? condition_(ctx) : true;
}
std::chrono::milliseconds functional_middleware::timeout() const
{
    return timeout_;
}

void functional_middleware::operator()(request_context &ctx, http::response<http::string_body> &res,
                                       std::function<void()> next)
{
    sync_func_(ctx, res, next);
}

void functional_middleware::operator()(request_context &ctx, http::response<http::string_body> &res,
                                       std::function<void()> next, async_middleware_callback callback)
{
    if (async_func_)
    {
        async_func_(ctx, res, next, callback);
    }
    else
    {
        // Fallback to sync implementation
        middleware::operator()(ctx, res, next, callback);
    }
}

}// namespace foxhttp