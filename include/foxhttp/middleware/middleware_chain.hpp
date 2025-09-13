/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <atomic>
#include <chrono>
#include <foxhttp/middleware/middleware.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace foxhttp {

class MiddlewareChain
{
public:
    using next_func = std::function<void()>;
    using error_handler =
            std::function<void(const RequestContext &, http::response<http::string_body> &, const std::exception &)>;
    using timeout_handler = std::function<void(const RequestContext &, http::response<http::string_body> &)>;

    MiddlewareChain() = default;
    void use(std::shared_ptr<Middleware> mw);
    void use(std::initializer_list<std::shared_ptr<Middleware>> mws);
    void insert(std::size_t index, std::shared_ptr<Middleware> mw);
    void set_error_handler(error_handler handler);
    void set_timeout_handler(timeout_handler handler);
    void set_timeout(std::chrono::milliseconds timeout);

    void execute(const RequestContext &ctx, http::response<http::string_body> &res);
    void execute_async(const RequestContext &ctx, http::response<http::string_body> &res,
                       std::function<void()> completion_callback = nullptr);
    void cancel();
    void clear();

    std::size_t size() const;
    bool empty() const;

private:
    void execute_next(const RequestContext &ctx, http::response<http::string_body> &res);
    void handle_error(const RequestContext &ctx, http::response<http::string_body> &res, const std::exception &e);
    void handle_timeout(const RequestContext &ctx, http::response<http::string_body> &res);

private:
    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<Middleware>> middlewares_;
    std::size_t current_index_ = 0;

    error_handler error_handler_;
    timeout_handler timeout_handler_;
    std::chrono::milliseconds timeout_{0};
    std::chrono::steady_clock::time_point execution_start_;
    std::atomic<bool> timed_out_{false};
    std::atomic<bool> cancelled_{false};
};

}// namespace foxhttp