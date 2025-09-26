/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <foxhttp/middleware/middleware.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace foxhttp {

class middleware_chain
{
public:
    using error_handler =
            std::function<void(request_context &, http::response<http::string_body> &, const std::exception &)>;
    using timeout_handler = std::function<void(request_context &, http::response<http::string_body> &)>;
    using completion_callback = std::function<void(middleware_result result, const std::string &error_message)>;

    explicit middleware_chain(boost::asio::io_context &io_context);
    middleware_chain() = default;

    void use(std::shared_ptr<middleware> mw);
    void use(std::initializer_list<std::shared_ptr<middleware>> mws);
    void insert(std::size_t index, std::shared_ptr<middleware> mw);
    void insert_by_priority(std::shared_ptr<middleware> mw);
    void remove(const std::string &middleware_name);
    void clear();

    void set_error_handler(error_handler handler);
    void set_timeout_handler(timeout_handler handler);
    void set_global_timeout(std::chrono::milliseconds timeout);
    void set_io_context(boost::asio::io_context &io_context);
    void enable_statistics(bool enable = true);

    void execute(request_context &ctx, http::response<http::string_body> &res);
    void execute_async(request_context &ctx, http::response<http::string_body> &res,
                       completion_callback callback = nullptr);

    void cancel();
    void pause();
    void resume();

    std::size_t size() const;
    bool empty() const;
    bool is_running() const;
    std::vector<std::string> get_middleware_names() const;

    std::unordered_map<std::string, middleware_stats> get_statistics() const;
    void reset_statistics();
    void print_statistics() const;

private:
    struct middleware_info
    {
        std::shared_ptr<middleware> middleware;
        std::chrono::steady_clock::time_point execution_start;
        std::shared_ptr<boost::asio::steady_timer> timeout_timer;
    };

    void _execute_next(request_context &ctx, http::response<http::string_body> &res);
    void _execute_next_async(request_context &ctx, http::response<http::string_body> &res,
                             completion_callback callback);
    void _handle_error(request_context &ctx, http::response<http::string_body> &res, const std::exception &e,
                       completion_callback callback = nullptr);
    void _handle_timeout(request_context &ctx, http::response<http::string_body> &res,
                         completion_callback callback = nullptr);
    void _handle_middleware_result(request_context &ctx, http::response<http::string_body> &res,
                                   middleware_result result, const std::string &error_message,
                                   completion_callback callback);

    void _sort_middlewares_by_priority();
    void _setup_middleware_timeout(std::shared_ptr<middleware> mw, request_context &ctx,
                                   http::response<http::string_body> &res, completion_callback callback);

private:
    mutable std::mutex mutex_;
    std::vector<middleware_info> middlewares_;
    std::size_t current_index_ = 0;
    bool statistics_enabled_ = true;
    bool auto_sort_by_priority_ = true;

    error_handler error_handler_;
    timeout_handler timeout_handler_;

    std::chrono::milliseconds global_timeout_{0};
    std::chrono::steady_clock::time_point execution_start_;

    std::atomic<bool> timed_out_{false};
    std::atomic<bool> cancelled_{false};
    std::atomic<bool> paused_{false};
    std::atomic<bool> running_{false};

    boost::asio::io_context *io_context_ = nullptr;
    std::shared_ptr<boost::asio::steady_timer> global_timeout_timer_;

    std::weak_ptr<middleware_info> current_middleware_;
};

}// namespace foxhttp