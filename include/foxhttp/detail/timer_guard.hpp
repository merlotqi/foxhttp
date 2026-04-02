#pragma once

#include <spdlog/spdlog.h>

#include <boost/asio/steady_timer.hpp>
#include <boost/system/system_error.hpp>

namespace foxhttp::detail {

class TimerGuard {
 public:
  explicit TimerGuard(boost::asio::steady_timer& timer) : timer_(timer) {}

  ~TimerGuard() {
    try {
      timer_.cancel();
    } catch (const boost::system::system_error& e) {
      spdlog::warn("Failed to cancel timer in TimerGuard: {}", e.what());
    }
  }

  TimerGuard(const TimerGuard&) = delete;
  TimerGuard& operator=(const TimerGuard&) = delete;

  TimerGuard(TimerGuard&&) = delete;
  TimerGuard& operator=(TimerGuard&&) = delete;

 private:
  boost::asio::steady_timer& timer_;
};

}  // namespace foxhttp::detail
