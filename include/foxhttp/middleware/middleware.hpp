/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <boost/asio.hpp>
#include <chrono>
#include <foxhttp/core/request_context.hpp>
#include <functional>
#include <memory>

namespace foxhttp {

class MiddlewareChain;

struct MiddlewareStats
{
    std::atomic<std::size_t> execution_count{0};
    std::atomic<std::chrono::microseconds> total_execution_time{std::chrono::microseconds{0}};
    std::atomic<std::size_t> error_count{0};
    std::atomic<std::size_t> timeout_count{0};

    MiddlewareStats() = default;
    MiddlewareStats(const MiddlewareStats &other);
    MiddlewareStats &operator=(const MiddlewareStats &other);

    void reset();
};

// Middleware priority levels
enum class middleware_priority : int
{
    lowest = -1000,
    low = -100,
    normal = 0,
    high = 100,
    highest = 1000
};

// Middleware execution result
enum class middleware_result
{
    continue_,// Continue to next middleware
    stop,     // Stop execution chain
    error,    // Error occurred
    timeout   // Timeout occurred
};

// Async middleware callback type
using AsyncMiddlewareCallback = std::function<void(middleware_result result, const std::string &error_message)>;

class Middleware
{
public:
    virtual ~Middleware() = default;

    virtual void operator()(RequestContext &ctx, http::response<http::string_body> &res,
                            std::function<void()> next) = 0;
    virtual void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next,
                            AsyncMiddlewareCallback callback);

    virtual middleware_priority priority() const;
    virtual std::string name() const;
    virtual bool should_execute(RequestContext &ctx) const;
    virtual std::chrono::milliseconds timeout() const;
    virtual MiddlewareStats &stats();
    virtual const MiddlewareStats &stats() const;

protected:
    MiddlewareStats stats_;
};


template<middleware_priority Priority>
class PriorityMiddleware : public Middleware
{
public:
    middleware_priority priority() const override
    {
        return Priority;
    }
};

class ConditionalMiddleware : public Middleware
{
public:
    using ConditionFunc = std::function<bool(RequestContext &)>;

    explicit ConditionalMiddleware(std::shared_ptr<Middleware> middleware, ConditionFunc condition);
    void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
    void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next,
                    AsyncMiddlewareCallback callback) override;

    middleware_priority priority() const override;
    std::string name() const override;
    bool should_execute(RequestContext &ctx) const override;
    std::chrono::milliseconds timeout() const override;
    MiddlewareStats &stats() override;
    const MiddlewareStats &stats() const override;

private:
    std::shared_ptr<Middleware> middleware_;
    ConditionFunc condition_;
};

}// namespace foxhttp