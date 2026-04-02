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

namespace foxhttp::server {

using TimerManagerConfig = config::TimerManagerConfig;

using TimerId = uint64_t;
using TimerCallback = std::function<void()>;
using TimerClock = std::chrono::steady_clock;
using TimerTimePoint = TimerClock::time_point;
using TimerDuration = std::chrono::milliseconds;

enum class TimerPriority {
  Low = 0,
  Normal = 1,
  High = 2,
  Critical = 3
};

struct TimerTask {
  TimerTimePoint expiry_time;
  TimerCallback callback;
  TimerDuration interval;
  bool is_repeating;
  TimerPriority priority;
  TimerId id;
  TimerTimePoint created_time;

  bool operator>(const TimerTask &other) const;
  bool is_expired(const TimerTimePoint &now) const;
  TimerTimePoint next_execution_time() const;
};

struct TimerStatistics {
  std::atomic<std::size_t> total_scheduled{0};
  std::atomic<std::size_t> total_executed{0};
  std::atomic<std::size_t> total_cancelled{0};
  std::atomic<std::size_t> total_failed{0};
  std::atomic<std::size_t> current_pending{0};
  std::atomic<std::chrono::microseconds> total_execution_time{std::chrono::microseconds{0}};

  TimerStatistics() = default;
  TimerStatistics(const TimerStatistics &rhs);
  TimerStatistics &operator=(const TimerStatistics &rhs);

  void reset();
  std::chrono::microseconds average_execution_time() const;
};

class TimerBucket {
 public:
  explicit TimerBucket(std::size_t bucket_id);
  ~TimerBucket();
  void start(boost::asio::io_context &io_context);
  void stop();
  bool add_task(const TimerTask &task);
  bool cancel_task(TimerId timer_id);
  std::optional<TimerTimePoint> get_next_expiry() const;
  std::size_t get_pending_count() const;

 private:
  void start_timer();
  void process_expired_tasks();
  void reschedule_timer();

 private:
  std::size_t bucket_id_;
  boost::asio::io_context *io_context_{nullptr};
  std::unique_ptr<boost::asio::steady_timer> timer_;
  std::unordered_map<TimerId, TimerTask> pending_tasks_;
  TimerTimePoint next_expiry_time_{TimerTimePoint::max()};
  mutable std::mutex mutex_;
  std::atomic<bool> is_running_;
  TimerStatistics statistics_;
};

class TimerManager {
 public:
  explicit TimerManager(boost::asio::io_context &io_context, const TimerManagerConfig &cfg = {});
  ~TimerManager();

  void start();
  void stop();

  TimerId schedule_at(TimerTimePoint when, TimerCallback callback,
                      TimerPriority priority = TimerPriority::Normal);
  TimerId schedule_after(TimerDuration delay, TimerCallback callback,
                         TimerPriority priority = TimerPriority::Normal);
  TimerId schedule_every(TimerDuration interval, TimerCallback callback,
                         TimerPriority priority = TimerPriority::Normal);
  bool cancel(TimerId timer_id);

  TimerStatistics get_statistics() const;
  const TimerManagerConfig &config() const;
  void update_config(const TimerManagerConfig &new_config);
  void perform_cleanup();
  void report_statistics();

 private:
  void initialize();
  TimerId schedule_impl(TimerTimePoint when, TimerDuration interval, TimerCallback callback, bool is_repeating,
                        TimerPriority priority);
  void start_cleanup_timer();
  void start_statistics_timer();

 private:
  boost::asio::io_context &io_context_;
  TimerManagerConfig config_;
  std::vector<std::unique_ptr<TimerBucket>> buckets_;
  std::atomic<TimerId> next_timer_id_;
  std::atomic<bool> is_running_;

  boost::asio::steady_timer cleanup_timer_;
  boost::asio::steady_timer statistics_timer_;

  TimerStatistics statistics_;
};

}  // namespace foxhttp::server
