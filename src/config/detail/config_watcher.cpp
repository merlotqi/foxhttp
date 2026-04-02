#include <atomic>
#include <chrono>
#include <filesystem>
#include <foxhttp/config/config_manager.hpp>
#include <foxhttp/config/detail/config_watcher.hpp>
#include <string>
#include <thread>

namespace foxhttp::config::detail {

ConfigWatcher::ConfigWatcher(std::string path, std::chrono::milliseconds interval)
    : path_(std::move(path)), interval_(interval), running_(false) {}

ConfigWatcher::~ConfigWatcher() { stop(); }

void ConfigWatcher::start() {
  if (running_.exchange(true)) return;
  last_write_time_ = last_write_time();
  worker_ = std::thread([this] { run_loop(); });
}

void ConfigWatcher::stop() {
  if (!running_.exchange(false)) return;
  if (worker_.joinable()) worker_.join();
}

std::filesystem::file_time_type ConfigWatcher::last_write_time() const {
  std::error_code ec;
  auto t = std::filesystem::last_write_time(path_, ec);
  if (ec) return {};
  return t;
}

void ConfigWatcher::run_loop() {
  auto debounce_until = std::chrono::steady_clock::time_point{};
  while (running_.load()) {
    auto now = std::chrono::steady_clock::now();
    if (now >= debounce_until) {
      auto t = last_write_time();
      if (t != std::filesystem::file_time_type{} && t != last_write_time_) {
        last_write_time_ = t;
        debounce_until = now + std::chrono::milliseconds(300);

        ConfigManager::instance().load_file(path_);
        on_reload();
      }
    }
    std::this_thread::sleep_for(interval_);
  }
}

}  // namespace foxhttp::config::detail
