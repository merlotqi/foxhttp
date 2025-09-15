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

class middleware_chain;

struct middleware_stats
{
    std::atomic<std::size_t> execution_count{0};
    std::atomic<std::chrono::microseconds> total_execution_time{std::chrono::microseconds{0}};
    std::atomic<std::size_t> error_count{0};
    std::atomic<std::size_t> timeout_count{0};

    middleware_stats() = default;
    middleware_stats(const middleware_stats &other);
    middleware_stats &operator=(const middleware_stats &other);

    void reset();
};

// middleware priority levels
enum class middleware_priority : int
{
    lowest = -1000,
    low = -100,
    normal = 0,
    high = 100,
    highest = 1000
};

// middleware execution result
enum class middleware_result
{
    continue_,// Continue to next middleware
    stop,     // Stop execution chain
    error,    // Error occurred
    timeout   // Timeout occurred
};

// Async middleware callback type
using async_middleware_callback = std::function<void(middleware_result result, const std::string &error_message)>;

class middleware
{
public:
    virtual ~middleware() = default;

    virtual void operator()(request_context &ctx, http::response<http::string_body> &res,
                            std::function<void()> next) = 0;
    virtual void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next,
                            async_middleware_callback callback);

    virtual middleware_priority priority() const;
    virtual std::string name() const;
    virtual bool should_execute(request_context &ctx) const;
    virtual std::chrono::milliseconds timeout() const;
    virtual middleware_stats &stats();
    virtual const middleware_stats &stats() const;

protected:
    middleware_stats stats_;
};


template<middleware_priority Priority>
class priority_middleware : public middleware
{
public:
    middleware_priority priority() const override
    {
        return Priority;
    }
};

class conditional_middleware : public middleware
{
public:
    using condition_func = std::function<bool(request_context &)>;

    explicit conditional_middleware(std::shared_ptr<middleware> middleware, condition_func condition);
    void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
    void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next,
                    async_middleware_callback callback) override;

    middleware_priority priority() const override;
    std::string name() const override;
    bool should_execute(request_context &ctx) const override;
    std::chrono::milliseconds timeout() const override;
    middleware_stats &stats() override;
    const middleware_stats &stats() const override;

private:
    std::shared_ptr<middleware> middleware_;
    condition_func condition_;
};

}// namespace foxhttp