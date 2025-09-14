/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <foxhttp/middleware/middleware.hpp>

namespace foxhttp {

/* ----------------------------- MiddlewareStats ---------------------------- */

MiddlewareStats::MiddlewareStats(const MiddlewareStats &other)
    : execution_count(other.execution_count.load()), total_execution_time(other.total_execution_time.load()),
      error_count(other.error_count.load()), timeout_count(other.timeout_count.load())
{
}

MiddlewareStats &MiddlewareStats::operator=(const MiddlewareStats &other)
{
    if (this != &other)
    {
        execution_count.store(other.execution_count.load());
        total_execution_time.store(other.total_execution_time.load());
        error_count.store(other.error_count.load());
        timeout_count.store(other.timeout_count.load());
    }
    return *this;
}

void MiddlewareStats::reset()
{
    execution_count = 0;
    total_execution_time = std::chrono::microseconds{0};
    error_count = 0;
    timeout_count = 0;
}

/* ------------------------------- Middleware ------------------------------- */

void Middleware::operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next,
                            AsyncMiddlewareCallback callback)
{
    // Default implementation: execute synchronously and call callback
    try
    {
        (*this)(ctx, res, next);
        callback(middleware_result::continue_, "");
    }
    catch (const std::exception &e)
    {
        callback(middleware_result::error, e.what());
    }
}

// Get middleware priority (default: normal)
middleware_priority Middleware::priority() const
{
    return middleware_priority::normal;
}

// Get middleware name for logging/debugging
std::string Middleware::name() const
{
    return "AnonymousMiddleware";
}

// Check if middleware should execute for this request
bool Middleware::should_execute(RequestContext &ctx) const
{
    return true;
}

// Get execution timeout for this middleware (0 = no timeout)
std::chrono::milliseconds Middleware::timeout() const
{
    return std::chrono::milliseconds{0};
}

// Get statistics
MiddlewareStats &Middleware::stats()
{
    return stats_;
}
const MiddlewareStats &Middleware::stats() const
{
    return stats_;
}

/* -------------------------- ConditionalMiddleware ------------------------- */

ConditionalMiddleware::ConditionalMiddleware(std::shared_ptr<Middleware> middleware, ConditionFunc condition)
    : middleware_(std::move(middleware)), condition_(std::move(condition))
{
}

void ConditionalMiddleware::operator()(RequestContext &ctx, http::response<http::string_body> &res,
                                       std::function<void()> next)
{
    if (condition_(ctx))
    {
        (*middleware_)(ctx, res, next);
    }
    else
    {
        next();
    }
}

void ConditionalMiddleware::operator()(RequestContext &ctx, http::response<http::string_body> &res,
                                       std::function<void()> next, AsyncMiddlewareCallback callback)
{
    if (condition_(ctx))
    {
        (*middleware_)(ctx, res, next, callback);
    }
    else
    {
        next();
        callback(middleware_result::continue_, "");
    }
}

middleware_priority ConditionalMiddleware::priority() const
{
    return middleware_->priority();
}
std::string ConditionalMiddleware::name() const
{
    return "Conditional(" + middleware_->name() + ")";
}
bool ConditionalMiddleware::should_execute(RequestContext &ctx) const
{
    return condition_(ctx);
}
std::chrono::milliseconds ConditionalMiddleware::timeout() const
{
    return middleware_->timeout();
}

MiddlewareStats &ConditionalMiddleware::stats()
{
    return middleware_->stats();
}
const MiddlewareStats &ConditionalMiddleware::stats() const
{
    return middleware_->stats();
}


}// namespace foxhttp