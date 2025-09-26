/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <foxhttp/server/signal_set.hpp>

namespace foxhttp {

signal_set::signal_set(boost::asio::io_context &io_context)
    : signals_(io_context), strand_(io_context.get_executor()), stopped_(false)
{
}

void signal_set::register_handler(int signal_number, signal_handler handler)
{
    std::lock_guard<std::mutex> lock(mutex_);
    signal_handlers_[signal_number].push_back(std::move(handler));
    signals_.add(signal_number);
}

void signal_set::set_error_handler(error_handler handler)
{
    std::lock_guard<std::mutex> lock(mutex_);
    error_handler_ = std::move(handler);
}

void signal_set::start()
{
    if (stopped_)
        return;
    _do_async_wait();
}

void signal_set::stop()
{
    stopped_ = true;
    signals_.cancel();
}

void signal_set::_do_async_wait()
{
    signals_.async_wait(
            boost::asio::bind_executor(strand_, [self = shared_from_this()](const std::error_code &ec, int sig) {
                if (ec)
                {
                    if (self->error_handler_)
                        self->error_handler_(ec, "async_wait");
                    return;
                }

                self->_handle_signal(sig);
                if (!self->stopped_)
                    self->_do_async_wait();
            }));
}

void signal_set::_handle_signal(int sig)
{
    std::vector<signal_handler> handlers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = signal_handlers_.find(sig);
        if (it != signal_handlers_.end())
            handlers = it->second;
    }

    for (auto &h: handlers) h(sig, _siganl_name(sig));
}

std::string signal_set::_siganl_name(int sig)
{
    static const std::unordered_map<int, std::string> names = {
            { SIGINT,  "SIGINT"},
            {SIGTERM, "SIGTERM"},
#ifndef _WIN32
            { SIGHUP,  "SIGHUP"},
            {SIGQUIT, "SIGQUIT"},
            {SIGUSR1, "SIGUSR1"},
            {SIGUSR2, "SIGUSR2"},
            {SIGPIPE, "SIGPIPE"},
            {SIGALRM, "SIGALRM"},
#endif
#ifdef _WIN32
#endif
    };
    auto it = names.find(sig);
    return it != names.end() ? it->second : "UNKNOWN";
}


}// namespace foxhttp