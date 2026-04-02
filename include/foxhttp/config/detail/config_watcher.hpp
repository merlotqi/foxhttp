#pragma once

#include <atomic>
#include <boost/signals2/signal.hpp>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

namespace foxhttp::config::detail {

class ConfigWatcher {
 public:
  explicit ConfigWatcher(std::string path, std::chrono::milliseconds interval = std::chrono::seconds(2));
  ~ConfigWatcher();

  // Signal for reload notifications
  boost::signals2::signal<void()> on_reload;

  void start();
  void stop();

 private:
  std::filesystem::file_time_type last_write_time() const;
  void run_loop();

 private:
  std::string path_;
  std::chrono::milliseconds interval_;
  std::atomic<bool> running_;
  std::filesystem::file_time_type last_write_time_{};
  std::thread worker_;
};

}  // namespace foxhttp::config::detail
