/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <atomic>
#include <chrono>
#include <filesystem>
#include <foxhttp/config/config_manager.hpp>
#include <foxhttp/config/details/config_watcher.hpp>
#include <functional>
#include <string>
#include <thread>

namespace foxhttp {
namespace details {

config_watcher::config_watcher(std::string path, std::chrono::milliseconds interval)
    : path_(std::move(path)), interval_(interval), running_(false)
{
}

config_watcher::~config_watcher()
{
    stop();
}

void config_watcher::start()
{
    if (running_.exchange(true))
        return;
    last_write_time_ = _last_write_time();
    worker_ = std::thread([this] { _run_loop(); });
}

void config_watcher::stop()
{
    if (!running_.exchange(false))
        return;
    if (worker_.joinable())
        worker_.join();
}

std::filesystem::file_time_type config_watcher::_last_write_time() const
{
    std::error_code ec;
    auto t = std::filesystem::last_write_time(path_, ec);
    if (ec)
        return {};
    return t;
}

void config_watcher::_run_loop()
{
    auto debounce_until = std::chrono::steady_clock::time_point{};
    while (running_.load())
    {
        auto now = std::chrono::steady_clock::now();
        if (now >= debounce_until)
        {
            auto t = _last_write_time();
            if (t != std::filesystem::file_time_type{} && t != last_write_time_)
            {
                last_write_time_ = t;
                debounce_until = now + std::chrono::milliseconds(300);

                config_manager::instance().load_file(path_);
                on_reload();
            }
        }
        std::this_thread::sleep_for(interval_);
    }
}


}// namespace details
}// namespace foxhttp
