#pragma once

#include <spdlog/spdlog.h>

#include <boost/asio/steady_timer.hpp>
#include <boost/system/system_error.hpp>

namespace foxhttp::detail {

class timer_guard {
 public:
  explicit timer_guard(boost::asio::steady_timer& timer) : timer_(timer) {}

  ~timer_guard() {
    try {
      timer_.cancel();
    } catch (const boost::system::system_error& e) {
      spdlog::warn("Failed to cancel timer in timer_guard: {}", e.what());
    }
  }

  timer_guard(const timer_guard&) = delete;
  timer_guard& operator=(const timer_guard&) = delete;

  timer_guard(timer_guard&&) = delete;
  timer_guard& operator=(timer_guard&&) = delete;

 private:
  boost::asio::steady_timer& timer_;
};

}  // namespace foxhttp::detail
