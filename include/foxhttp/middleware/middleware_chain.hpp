/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
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

class MiddlewareChain
{
public:
    using errorHandler =
            std::function<void(RequestContext &, http::response<http::string_body> &, const std::exception &)>;
    using timeoutHandler = std::function<void(RequestContext &, http::response<http::string_body> &)>;
    using completionCallback = std::function<void(middleware_result result, const std::string &error_message)>;

    explicit MiddlewareChain(boost::asio::io_context &io_context);
    MiddlewareChain() = default;

    // Middleware management
    void use(std::shared_ptr<Middleware> mw);
    void use(std::initializer_list<std::shared_ptr<Middleware>> mws);
    void insert(std::size_t index, std::shared_ptr<Middleware> mw);
    void insert_by_priority(std::shared_ptr<Middleware> mw);
    void remove(const std::string &middleware_name);
    void clear();

    // Configuration
    void set_error_handler(errorHandler handler);
    void set_timeout_handler(timeoutHandler handler);
    void set_global_timeout(std::chrono::milliseconds timeout);
    void set_io_context(boost::asio::io_context &io_context);
    void enable_statistics(bool enable = true);

    // Execution methods
    void execute(RequestContext &ctx, http::response<http::string_body> &res);
    void execute_async(RequestContext &ctx, http::response<http::string_body> &res,
                       completionCallback callback = nullptr);

    // Control methods
    void cancel();
    void pause();
    void resume();

    // Information methods
    std::size_t size() const;
    bool empty() const;
    bool is_running() const;
    std::vector<std::string> get_middleware_names() const;

    // Statistics
    std::unordered_map<std::string, MiddlewareStats> get_statistics() const;
    void reset_statistics();
    void print_statistics() const;

private:
    struct MiddlewareInfo
    {
        std::shared_ptr<Middleware> middleware;
        std::chrono::steady_clock::time_point execution_start;
        std::shared_ptr<boost::asio::steady_timer> timeout_timer;
    };

    void execute_next(RequestContext &ctx, http::response<http::string_body> &res);
    void execute_next_async(RequestContext &ctx, http::response<http::string_body> &res, completionCallback callback);
    void handle_error(RequestContext &ctx, http::response<http::string_body> &res, const std::exception &e,
                      completionCallback callback = nullptr);
    void handle_timeout(RequestContext &ctx, http::response<http::string_body> &res,
                        completionCallback callback = nullptr);
    void handle_middleware_result(RequestContext &ctx, http::response<http::string_body> &res, middleware_result result,
                                  const std::string &error_message, completionCallback callback);

    void sort_middlewares_by_priority();
    void setup_middleware_timeout(std::shared_ptr<Middleware> mw, RequestContext &ctx,
                                  http::response<http::string_body> &res, completionCallback callback);

private:
    mutable std::mutex mutex_;
    std::vector<MiddlewareInfo> middlewares_;
    std::size_t current_index_ = 0;
    bool statistics_enabled_ = true;
    bool auto_sort_by_priority_ = true;

    // Handlers
    errorHandler error_handler_;
    timeoutHandler timeout_handler_;

    // Timeouts
    std::chrono::milliseconds global_timeout_{0};
    std::chrono::steady_clock::time_point execution_start_;

    // State management
    std::atomic<bool> timed_out_{false};
    std::atomic<bool> cancelled_{false};
    std::atomic<bool> paused_{false};
    std::atomic<bool> running_{false};

    // Async support
    boost::asio::io_context *io_context_ = nullptr;
    std::shared_ptr<boost::asio::steady_timer> global_timeout_timer_;

    // Current execution context
    std::weak_ptr<MiddlewareInfo> current_middleware_;
};

}// namespace foxhttp