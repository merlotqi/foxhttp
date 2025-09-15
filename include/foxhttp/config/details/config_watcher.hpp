/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */


#pragma once

#include <atomic>
#include <boost/signals2/signal.hpp>
#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <thread>

namespace foxhttp {
namespace details {

class config_watcher
{
public:
    explicit config_watcher(std::string path, std::chrono::milliseconds interval = std::chrono::seconds(2));
    ~config_watcher();

    // Signal for reload notifications
    boost::signals2::signal<void()> on_reload;

    void start();
    void stop();

private:
    std::filesystem::file_time_type _last_write_time() const;
    void _run_loop();

private:
    std::string path_;
    std::chrono::milliseconds interval_;
    std::atomic<bool> running_;
    std::filesystem::file_time_type last_write_time_{};
    std::thread worker_;
};

}// namespace details
}// namespace foxhttp
