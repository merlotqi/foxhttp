#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <foxhttp/config/configs.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace foxhttp {

using timer_manager_config = ::foxhttp::timer_manager_config;

using timer_id_t = uint64_t;
using timer_callback_t = std::function<void()>;
using clock_t = std::chrono::steady_clock;
using time_point_t = clock_t::time_point;
using duration_t = std::chrono::milliseconds;

enum class timer_priority {
  low = 0,
  normal = 1,
  high = 2,
  critical = 3
};

struct timer_task {
  time_point_t expiry_time;
  timer_callback_t callback;
  duration_t interval;
  bool is_repeating;
  timer_priority priority;
  timer_id_t id;
  std::chrono::steady_clock::time_point created_time;

  bool operator>(const timer_task &other) const;
  bool is_expired(const time_point_t &now) const;
  time_point_t next_execution_time() const;
};

struct timer_statistics {
  std::atomic<std::size_t> total_scheduled{0};
  std::atomic<std::size_t> total_executed{0};
  std::atomic<std::size_t> total_cancelled{0};
  std::atomic<std::size_t> total_failed{0};
  std::atomic<std::size_t> current_pending{0};
  std::atomic<std::chrono::microseconds> total_execution_time{std::chrono::microseconds{0}};

  timer_statistics() = default;
  timer_statistics(const timer_statistics &rhs);
  timer_statistics &operator=(const timer_statistics &rhs);

  void reset();
  std::chrono::microseconds average_execution_time() const;
};

class timer_bucket {
 public:
  explicit timer_bucket(std::size_t bucket_id);
  ~timer_bucket();
  void start(boost::asio::io_context &io_context);
  void stop();
  bool add_task(const timer_task &task);
  bool cancel_task(timer_id_t timer_id);
  std::optional<time_point_t> get_next_expiry() const;
  std::size_t get_pending_count() const;

 private:
  void start_timer();
  void process_expired_tasks();
  void reschedule_timer();

 private:
  std::size_t bucket_id_;
  boost::asio::io_context *io_context_{nullptr};
  std::unique_ptr<boost::asio::steady_timer> timer_;
  std::unordered_map<timer_id_t, timer_task> pending_tasks_;
  time_point_t next_expiry_time_{time_point_t::max()};
  mutable std::mutex mutex_;
  std::atomic<bool> is_running_;
  timer_statistics statistics_;
};

class timer_manager {
 public:
  explicit timer_manager(boost::asio::io_context &io_context, const timer_manager_config &cfg = {});
  ~timer_manager();

  void start();
  void stop();

  timer_id_t schedule_at(time_point_t when, timer_callback_t callback,
                         timer_priority priority = timer_priority::normal);
  timer_id_t schedule_after(duration_t delay, timer_callback_t callback,
                            timer_priority priority = timer_priority::normal);
  timer_id_t schedule_every(duration_t interval, timer_callback_t callback,
                            timer_priority priority = timer_priority::normal);
  bool cancel(timer_id_t timer_id);

  timer_statistics get_statistics() const;
  const timer_manager_config &config() const;
  void update_config(const timer_manager_config &new_config);
  void perform_cleanup();
  void report_statistics();

 private:
  void initialize();
  timer_id_t schedule_impl(time_point_t when, duration_t interval, timer_callback_t callback, bool is_repeating,
                           timer_priority priority);
  void start_cleanup_timer();
  void start_statistics_timer();

 private:
  boost::asio::io_context &io_context_;
  timer_manager_config config_;
  std::vector<std::unique_ptr<timer_bucket>> buckets_;
  std::atomic<timer_id_t> next_timer_id_;
  std::atomic<bool> is_running_;

  boost::asio::steady_timer cleanup_timer_;
  boost::asio::steady_timer statistics_timer_;

  timer_statistics statistics_;
};

}  // namespace foxhttp
