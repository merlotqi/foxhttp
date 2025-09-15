/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

namespace foxhttp {

middleware_chain::middleware_chain(boost::asio::io_context &io_context) : io_context_(&io_context) {}

// middleware management
void middleware_chain::use(std::shared_ptr<middleware> mw)
{
    std::lock_guard<std::mutex> lock(mutex_);
    middlewares_.push_back({std::move(mw), {}, nullptr});
    if (auto_sort_by_priority_)
    {
        _sort_middlewares_by_priority();
    }
}

void middleware_chain::use(std::initializer_list<std::shared_ptr<middleware>> mws)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &mw: mws)
    {
        middlewares_.push_back({std::move(mw), {}, nullptr});
    }
    if (auto_sort_by_priority_)
    {
        _sort_middlewares_by_priority();
    }
}

void middleware_chain::insert(std::size_t index, std::shared_ptr<middleware> mw)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (index >= middlewares_.size())
    {
        middlewares_.push_back({std::move(mw), {}, nullptr});
    }
    else
    {
        middlewares_.insert(middlewares_.begin() + index, {std::move(mw), {}, nullptr});
    }
}

void middleware_chain::insert_by_priority(std::shared_ptr<middleware> mw)
{
    std::lock_guard<std::mutex> lock(mutex_);
    middlewares_.push_back({std::move(mw), {}, nullptr});
    _sort_middlewares_by_priority();
}

void middleware_chain::remove(const std::string &middleware_name)
{
    std::lock_guard<std::mutex> lock(mutex_);
    middlewares_.erase(std::remove_if(middlewares_.begin(), middlewares_.end(),
                                      [&middleware_name](const middleware_info &info) {
                                          return info.middleware->name() == middleware_name;
                                      }),
                       middlewares_.end());
}

void middleware_chain::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    middlewares_.clear();
    current_index_ = 0;
}

// Configuration
void middleware_chain::set_error_handler(error_handler handler)
{
    std::lock_guard<std::mutex> lock(mutex_);
    error_handler_ = std::move(handler);
}

void middleware_chain::set_timeout_handler(timeout_handler handler)
{
    std::lock_guard<std::mutex> lock(mutex_);
    timeout_handler_ = std::move(handler);
}

void middleware_chain::set_global_timeout(std::chrono::milliseconds timeout)
{
    global_timeout_ = timeout;
}

void middleware_chain::set_io_context(boost::asio::io_context &io_context)
{
    io_context_ = &io_context;
}

void middleware_chain::enable_statistics(bool enable)
{
    statistics_enabled_ = enable;
}

// Execution methods
void middleware_chain::execute(request_context &ctx, http::response<http::string_body> &res)
{
    if (middlewares_.empty())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    current_index_ = 0;
    execution_start_ = std::chrono::steady_clock::now();
    timed_out_ = false;
    cancelled_ = false;
    paused_ = false;
    running_ = true;

    _execute_next(ctx, res);
}

void middleware_chain::execute_async(request_context &ctx, http::response<http::string_body> &res,
                                     completion_callback callback)
{
    if (middlewares_.empty())
    {
        if (callback)
            callback(middleware_result::continue_, "");
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    current_index_ = 0;
    execution_start_ = std::chrono::steady_clock::now();
    timed_out_ = false;
    cancelled_ = false;
    paused_ = false;
    running_ = true;

    // Setup global timeout if specified
    if (global_timeout_ > std::chrono::milliseconds{0} && io_context_)
    {
        global_timeout_timer_ = std::make_shared<boost::asio::steady_timer>(*io_context_, global_timeout_);
        global_timeout_timer_->async_wait([this, &ctx, &res, callback](boost::system::error_code ec) {
            if (!ec && !timed_out_.exchange(true))
            {
                _handle_timeout(ctx, res, callback);
            }
        });
    }

    _execute_next_async(ctx, res, callback);
}

// Control methods
void middleware_chain::cancel()
{
    std::lock_guard<std::mutex> lock(mutex_);
    cancelled_ = true;
    running_ = false;

    // Cancel global timeout timer
    if (global_timeout_timer_)
    {
        global_timeout_timer_->cancel();
    }

    // Cancel current middleware timeout timer
    if (auto current = current_middleware_.lock())
    {
        if (current->timeout_timer)
        {
            current->timeout_timer->cancel();
        }
    }
}

void middleware_chain::pause()
{
    paused_ = true;
}

void middleware_chain::resume()
{
    paused_ = false;
}

// Information methods
std::size_t middleware_chain::size() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return middlewares_.size();
}

bool middleware_chain::empty() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return middlewares_.empty();
}

bool middleware_chain::is_running() const
{
    return running_;
}

std::vector<std::string> middleware_chain::get_middleware_names() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    names.reserve(middlewares_.size());
    for (const auto &info: middlewares_)
    {
        names.push_back(info.middleware->name());
    }
    return names;
}

// Statistics
std::unordered_map<std::string, middleware_stats> middleware_chain::get_statistics() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, middleware_stats> stats;
    for (const auto &info: middlewares_)
    {
        stats[info.middleware->name()] = info.middleware->stats();
    }
    return stats;
}

void middleware_chain::reset_statistics()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &info: middlewares_)
    {
        info.middleware->stats().reset();
    }
}

void middleware_chain::print_statistics() const
{
    auto stats = get_statistics();
    std::cout << "=== middleware Statistics ===" << std::endl;
    for (const auto &[name, stat]: stats)
    {
        std::cout << "middleware: " << name << std::endl;
        std::cout << "  Executions: " << stat.execution_count.load() << std::endl;
        std::cout << "  Total Time: " << stat.total_execution_time.load().count() << " μs" << std::endl;
        std::cout << "  Errors: " << stat.error_count.load() << std::endl;
        std::cout << "  Timeouts: " << stat.timeout_count.load() << std::endl;
        std::cout << std::endl;
    }
}

// Private methods
void middleware_chain::_execute_next(request_context &ctx, http::response<http::string_body> &res)
{
    if (cancelled_ || timed_out_ || paused_ || current_index_ >= middlewares_.size())
    {
        running_ = false;
        return;
    }

    // Check global timeout
    if (global_timeout_ > std::chrono::milliseconds{0})
    {
        auto now = std::chrono::steady_clock::now();
        if (now - execution_start_ > global_timeout_)
        {
            _handle_timeout(ctx, res);
            return;
        }
    }

    auto &current_info = middlewares_[current_index_++];
    current_middleware_ =
            std::weak_ptr<middleware_info>(std::shared_ptr<middleware_info>(&current_info, [](middleware_info *) {}));

    // Check if middleware should execute
    if (!current_info.middleware->should_execute(ctx))
    {
        _execute_next(ctx, res);
        return;
    }

    // Record execution start time for statistics
    if (statistics_enabled_)
    {
        current_info.execution_start = std::chrono::steady_clock::now();
    }

    auto next = [this, &ctx, &res]() { _execute_next(ctx, res); };

    try
    {
        (*current_info.middleware)(ctx, res, next);

        // Update statistics
        if (statistics_enabled_)
        {
            auto execution_time = std::chrono::steady_clock::now() - current_info.execution_start;
            current_info.middleware->stats().execution_count++;
            auto current_time = current_info.middleware->stats().total_execution_time.load();
            auto new_time = current_time + std::chrono::duration_cast<std::chrono::microseconds>(execution_time);
            current_info.middleware->stats().total_execution_time.store(new_time);
        }
    }
    catch (const std::exception &e)
    {
        if (statistics_enabled_)
        {
            current_info.middleware->stats().error_count++;
        }
        _handle_error(ctx, res, e);
    }
}

void middleware_chain::_execute_next_async(request_context &ctx, http::response<http::string_body> &res,
                                           completion_callback callback)
{
    if (cancelled_ || timed_out_ || paused_ || current_index_ >= middlewares_.size())
    {
        running_ = false;
        if (callback)
        {
            callback(cancelled_   ? middleware_result::stop
                     : timed_out_ ? middleware_result::timeout
                                  : middleware_result::continue_,
                     "");
        }
        return;
    }

    auto &current_info = middlewares_[current_index_++];
    current_middleware_ =
            std::weak_ptr<middleware_info>(std::shared_ptr<middleware_info>(&current_info, [](middleware_info *) {}));

    // Check if middleware should execute
    if (!current_info.middleware->should_execute(ctx))
    {
        _execute_next_async(ctx, res, callback);
        return;
    }

    // Record execution start time for statistics
    if (statistics_enabled_)
    {
        current_info.execution_start = std::chrono::steady_clock::now();
    }

    // Setup middleware-specific timeout
    auto middleware_timeout = current_info.middleware->timeout();
    if (middleware_timeout > std::chrono::milliseconds{0} && io_context_)
    {
        _setup_middleware_timeout(current_info.middleware, ctx, res, callback);
    }

    auto next = [this, &ctx, &res, callback]() { _execute_next_async(ctx, res, callback); };

    auto async_callback = [this, &current_info, &ctx, &res, callback](middleware_result result,
                                                                      const std::string &error_message) {
        // Update statistics
        if (statistics_enabled_)
        {
            auto execution_time = std::chrono::steady_clock::now() - current_info.execution_start;
            current_info.middleware->stats().execution_count++;
            auto current_time = current_info.middleware->stats().total_execution_time.load();
            auto new_time = current_time + std::chrono::duration_cast<std::chrono::microseconds>(execution_time);
            current_info.middleware->stats().total_execution_time.store(new_time);

            if (result == middleware_result::error)
            {
                current_info.middleware->stats().error_count++;
            }
            else if (result == middleware_result::timeout)
            {
                current_info.middleware->stats().timeout_count++;
            }
        }

        _handle_middleware_result(ctx, res, result, error_message, callback);
    };

    try
    {
        (*current_info.middleware)(ctx, res, next, async_callback);
    }
    catch (const std::exception &e)
    {
        if (statistics_enabled_)
        {
            current_info.middleware->stats().error_count++;
        }
        _handle_error(ctx, res, e, callback);
    }
}

void middleware_chain::_handle_error(request_context &ctx, http::response<http::string_body> &res,
                                     const std::exception &e, completion_callback callback)
{
    if (error_handler_)
    {
        error_handler_(ctx, res, e);
    }
    else
    {
        res.result(http::status::internal_server_error);
        res.set(http::field::content_type, "text/plain");
        res.body() = "middleware error: " + std::string(e.what());
    }

    running_ = false;
    if (callback)
    {
        callback(middleware_result::error, e.what());
    }
}

void middleware_chain::_handle_timeout(request_context &ctx, http::response<http::string_body> &res,
                                       completion_callback callback)
{
    timed_out_ = true;
    running_ = false;

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

    if (callback)
    {
        callback(middleware_result::timeout, "Request timeout");
    }
}

void middleware_chain::_handle_middleware_result(request_context &ctx, http::response<http::string_body> &res,
                                                 middleware_result result, const std::string &error_message,
                                                 completion_callback callback)
{
    switch (result)
    {
        case middleware_result::continue_:
            _execute_next_async(ctx, res, callback);
            break;
        case middleware_result::stop:
            running_ = false;
            if (callback)
                callback(middleware_result::stop, "");
            break;
        case middleware_result::error:
            running_ = false;
            if (callback)
                callback(middleware_result::error, error_message);
            break;
        case middleware_result::timeout:
            running_ = false;
            if (callback)
                callback(middleware_result::timeout, error_message);
            break;
    }
}

void middleware_chain::_sort_middlewares_by_priority()
{
    std::sort(middlewares_.begin(), middlewares_.end(), [](const middleware_info &a, const middleware_info &b) {
        return static_cast<int>(a.middleware->priority()) < static_cast<int>(b.middleware->priority());
    });
}

void middleware_chain::_setup_middleware_timeout(std::shared_ptr<middleware> mw, request_context &ctx,
                                                 http::response<http::string_body> &res, completion_callback callback)
{
    if (!io_context_)
        return;

    auto timeout = mw->timeout();
    auto timer = std::make_shared<boost::asio::steady_timer>(*io_context_, timeout);

    timer->async_wait([this, &ctx, &res, callback, mw](boost::system::error_code ec) {
        if (!ec && !timed_out_.exchange(true))
        {
            if (statistics_enabled_)
            {
                mw->stats().timeout_count++;
            }
            _handle_timeout(ctx, res, callback);
        }
    });

    // Store timer reference in current middleware info
    if (auto current = current_middleware_.lock())
    {
        current->timeout_timer = timer;
    }
}

}// namespace foxhttp