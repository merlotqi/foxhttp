/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <atomic>
#include <chrono>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace foxhttp {

void MiddlewareChain::use(std::shared_ptr<Middleware> mw)
{
    std::lock_guard<std::mutex> lock(mutex_);
    middlewares_.push_back(std::move(mw));
}

void MiddlewareChain::use(std::initializer_list<std::shared_ptr<Middleware>> mws)
{
    std::lock_guard<std::mutex> lock(mutex_);
    middlewares_.insert(middlewares_.end(), mws.begin(), mws.end());
}

void MiddlewareChain::insert(std::size_t index, std::shared_ptr<Middleware> mw)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (index >= middlewares_.size())
    {
        middlewares_.push_back(std::move(mw));
    }
    else
    {
        middlewares_.insert(middlewares_.begin() + index, std::move(mw));
    }
}

void MiddlewareChain::set_error_handler(error_handler handler)
{
    std::lock_guard<std::mutex> lock(mutex_);
    error_handler_ = std::move(handler);
}

void MiddlewareChain::set_timeout_handler(timeout_handler handler)
{
    std::lock_guard<std::mutex> lock(mutex_);
    timeout_handler_ = std::move(handler);
}

void MiddlewareChain::set_timeout(std::chrono::milliseconds timeout)
{
    timeout_ = timeout;
}

void MiddlewareChain::execute(const RequestContext &ctx, http::response<http::string_body> &res)
{
    if (middlewares_.empty())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    current_index_ = 0;
    execution_start_ = std::chrono::steady_clock::now();
    timed_out_ = false;

    execute_next(ctx, res);
}

void MiddlewareChain::execute_async(const RequestContext &ctx, http::response<http::string_body> &res,
                                    std::function<void()> completion_callback)
{
    execute(ctx, res);
    if (completion_callback)
    {
        completion_callback();
    }
}

void MiddlewareChain::cancel()
{
    std::lock_guard<std::mutex> lock(mutex_);
    cancelled_ = true;
}

void MiddlewareChain::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    middlewares_.clear();
    current_index_ = 0;
}

std::size_t MiddlewareChain::size() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return middlewares_.size();
}

bool MiddlewareChain::empty() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return middlewares_.empty();
}

void MiddlewareChain::execute_next(const RequestContext &ctx, http::response<http::string_body> &res)
{
    if (cancelled_ || timed_out_ || current_index_ >= middlewares_.size())
    {
        return;
    }

    if (timeout_ > std::chrono::milliseconds(0))
    {
        auto now = std::chrono::steady_clock::now();
        if (now - execution_start_ > timeout_)
        {
            handle_timeout(ctx, res);
            return;
        }
    }

    auto &current_mw = middlewares_[current_index_++];

    auto next = [this, &ctx, &res]() { execute_next(ctx, res); };

    try
    {
        (*current_mw)(ctx, res, next);
    }
    catch (const std::exception &e)
    {
        handle_error(ctx, res, e);
    }
}

void MiddlewareChain::handle_error(const RequestContext &ctx, http::response<http::string_body> &res,
                                   const std::exception &e)
{
    if (error_handler_)
    {
        error_handler_(ctx, res, e);
    }
    else
    {
        res.result(http::status::internal_server_error);
        res.set(http::field::content_type, "text/plain");
        res.body() = "Middleware error: " + std::string(e.what());
    }
}

void MiddlewareChain::handle_timeout(const RequestContext &ctx, http::response<http::string_body> &res)
{
    timed_out_ = true;
    if (timeout_handler_)
    {
        timeout_handler_(ctx, res);
    }
    else
    {
        res.result(http::status::gateway_timeout);
        res.set(http::field::content_type, "text/plain");
        res.body() = "Request timeout";
    }
}

}// namespace foxhttp