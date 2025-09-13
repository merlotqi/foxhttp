/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <foxhttp/core/signal_set.hpp>

namespace foxhttp {

SignalSet::SignalSet(boost::asio::io_context &io_context)
    : signals_(io_context), strand_(io_context.get_executor()), stopped_(false)
{
}

void SignalSet::register_handler(int signal_number, SignalHandler handler)
{
    std::lock_guard<std::mutex> lock(mutex_);
    signal_handlers_[signal_number].push_back(std::move(handler));
    signals_.add(signal_number);
}

void SignalSet::set_error_handler(ErrorHandler handler)
{
    std::lock_guard<std::mutex> lock(mutex_);
    error_handler_ = std::move(handler);
}

void SignalSet::start()
{
    if (stopped_)
        return;
    do_async_wait();
}

void SignalSet::stop()
{
    stopped_ = true;
    signals_.cancel();
}

void SignalSet::do_async_wait()
{
    signals_.async_wait(
            boost::asio::bind_executor(strand_, [self = shared_from_this()](const std::error_code &ec, int sig) {
                if (ec)
                {
                    if (self->error_handler_)
                        self->error_handler_(ec, "async_wait");
                    return;
                }

                self->handle_signal(sig);
                if (!self->stopped_)
                    self->do_async_wait();
            }));
}

void SignalSet::handle_signal(int sig)
{
    std::vector<SignalHandler> handlers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = signal_handlers_.find(sig);
        if (it != signal_handlers_.end())
            handlers = it->second;
    }

    for (auto &h: handlers) h(sig, siganl_name(sig));
}

std::string SignalSet::siganl_name(int sig)
{
    static const std::unordered_map<int, std::string> names = {
            { SIGINT,  "SIGINT"},
            {SIGTERM, "SIGTERM"},
            { SIGHUP,  "SIGHUP"},
            {SIGQUIT, "SIGQUIT"},
            {SIGUSR1, "SIGUSR1"},
            {SIGUSR2, "SIGUSR2"}
    };
    auto it = names.find(sig);
    return it != names.end() ? it->second : "UNKNOWN";
}


}// namespace foxhttp