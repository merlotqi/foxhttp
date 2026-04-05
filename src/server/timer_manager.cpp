#include <algorithm>
#include <foxhttp/server/timer_manager.hpp>
#include <iostream>

namespace foxhttp::server {

std::string get_priority_name(TimerPriority priority) {
  switch (priority) {
    case TimerPriority::Low:
      return "Low";
    case TimerPriority::Normal:
      return "Normal";
    case TimerPriority::High:
      return "High";
    case TimerPriority::Critical:
      return "Critical";
    default:
      return "Unknown";
  }
}

void print_timer_statistics(const TimerManager &manager) {
  auto format_duration = [](std::chrono::microseconds duration) -> std::string {
    if (duration.count() < 1000) {
      return std::to_string(duration.count()) + "μs";
    } else if (duration.count() < 1000000) {
      return std::to_string(duration.count() / 1000) + "ms";
    } else {
      return std::to_string(duration.count() / 1000000) + "s";
    }
  };

  auto stats = manager.get_statistics();
  std::cout << "=== Timer Manager Statistics ===" << std::endl;
  std::cout << "Total Scheduled: " << stats.total_scheduled.load() << std::endl;
  std::cout << "Total Executed: " << stats.total_executed.load() << std::endl;
  std::cout << "Total Cancelled: " << stats.total_cancelled.load() << std::endl;
  std::cout << "Total Failed: " << stats.total_failed.load() << std::endl;
  std::cout << "Current Pending: " << stats.current_pending.load() << std::endl;
  std::cout << "Average Execution Time: " << format_duration(stats.average_execution_time()) << std::endl;
  std::cout << "================================" << std::endl;
}

/* ------------------------------- timer_task ------------------------------- */

bool TimerTask::operator>(const TimerTask &other) const {
  if (priority != other.priority) {
    return static_cast<int>(priority) < static_cast<int>(other.priority);
  }
  return expiry_time > other.expiry_time;
}

bool TimerTask::is_expired(const TimerTimePoint &now) const { return expiry_time <= now; }

TimerTimePoint TimerTask::next_execution_time() const { return expiry_time + interval; }

/* ---------------------------- timer_statistics ---------------------------- */

TimerStatistics::TimerStatistics(const TimerStatistics &rhs) {
  total_scheduled.store(rhs.total_scheduled.load());
  total_executed.store(rhs.total_executed.load());
  total_cancelled.store(rhs.total_cancelled.load());
  total_failed.store(rhs.total_failed.load());
  current_pending.store(rhs.current_pending.load());
  total_execution_time.store(rhs.total_execution_time.load());
}

TimerStatistics &TimerStatistics::operator=(const TimerStatistics &rhs) {
  if (this == &rhs) return *this;

  total_scheduled.store(rhs.total_scheduled.load());
  total_executed.store(rhs.total_executed.load());
  total_cancelled.store(rhs.total_cancelled.load());
  total_failed.store(rhs.total_failed.load());
  current_pending.store(rhs.current_pending.load());
  total_execution_time.store(rhs.total_execution_time.load());

  return *this;
}

void TimerStatistics::reset() {
  total_scheduled = 0;
  total_executed = 0;
  total_cancelled = 0;
  total_failed = 0;
  current_pending = 0;
  total_execution_time = std::chrono::microseconds{0};
}

std::chrono::microseconds TimerStatistics::average_execution_time() const {
  auto executed = total_executed.load();
  if (executed == 0) return std::chrono::microseconds{0};

  auto total = total_execution_time.load();
  return std::chrono::microseconds{total.count() / executed};
}

/* ------------------------------ timer_bucket ------------------------------ */

TimerBucket::TimerBucket(std::size_t bucket_id) : bucket_id_(bucket_id), is_running_(false) {}

TimerBucket::~TimerBucket() { stop(); }

void TimerBucket::start(boost::asio::io_context &io_context) {
  if (is_running_.exchange(true)) return;

  io_context_ = &io_context;
  timer_ = std::make_unique<boost::asio::steady_timer>(io_context);
  start_timer();
}

void TimerBucket::stop() {
  if (!is_running_.exchange(false)) return;

  if (timer_) {
    timer_->cancel();
  }

  std::lock_guard<std::mutex> lock(mutex_);
  pending_tasks_.clear();
}

bool TimerBucket::add_task(const TimerTask &task) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!is_running_.load()) return false;

  pending_tasks_.emplace(task.id, task);

  if (task.expiry_time < next_expiry_time_) {
    reschedule_timer();
  }

  return true;
}

bool TimerBucket::cancel_task(TimerId timer_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  return pending_tasks_.erase(timer_id) > 0;
}

std::optional<TimerTimePoint> TimerBucket::get_next_expiry() const {
  std::lock_guard<std::mutex> lock(mutex_);

  if (pending_tasks_.empty()) {
    return std::nullopt;
  }

  TimerTimePoint next_expiry = TimerTimePoint::max();
  for (const auto &[id, task] : pending_tasks_) {
    if (task.expiry_time < next_expiry) {
      next_expiry = task.expiry_time;
    }
  }

  return next_expiry;
}

std::size_t TimerBucket::get_pending_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return pending_tasks_.size();
}

void TimerBucket::start_timer() {
  auto next_expiry = get_next_expiry();
  if (!next_expiry) {
    return;
  }

  auto now = TimerClock::now();
  if (*next_expiry <= now) {
    process_expired_tasks();
  } else {
    timer_->expires_at(*next_expiry);
    timer_->async_wait([this](const std::error_code &ec) {
      if (!ec && is_running_.load()) {
        process_expired_tasks();
        start_timer();
      }
    });
  }
}

void TimerBucket::process_expired_tasks() {
  std::vector<TimerTask> expired_tasks;
  auto now = TimerClock::now();

  {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = pending_tasks_.begin(); it != pending_tasks_.end();) {
      if (it->second.is_expired(now)) {
        expired_tasks.push_back(it->second);
        it = pending_tasks_.erase(it);
      } else {
        ++it;
      }
    }

    std::sort(expired_tasks.begin(), expired_tasks.end(),
              [](const TimerTask &a, const TimerTask &b) { return a.priority > b.priority; });
  }

  for (auto &task : expired_tasks) {
    auto start_time = TimerClock::now();

    try {
      if (task.callback) {
        task.callback();
      }
    } catch (const std::exception &e) {
      std::cerr << "Timer task error in bucket " << bucket_id_ << ": " << e.what() << std::endl;
    }

    auto execution_time = std::chrono::duration_cast<std::chrono::microseconds>(TimerClock::now() - start_time);

    statistics_.total_executed.fetch_add(1, std::memory_order_relaxed);
    {
      auto old_value = statistics_.total_execution_time.load(std::memory_order_relaxed);
      while (!statistics_.total_execution_time.compare_exchange_weak(
          old_value, std::chrono::microseconds{old_value.count() + execution_time.count()}, std::memory_order_relaxed,
          std::memory_order_relaxed)) {
      }
    }

    if (task.is_repeating) {
      std::lock_guard<std::mutex> lock(mutex_);
      TimerTask new_task = task;
      new_task.expiry_time = task.next_execution_time();
      pending_tasks_.emplace(new_task.id, new_task);
    }
  }
}

void TimerBucket::reschedule_timer() {
  if (timer_) {
    timer_->cancel();
  }
  start_timer();
}

/* ------------------------------ TimerManager ----------------------------- */

TimerManager::TimerManager(boost::asio::io_context &io_context, const TimerManagerConfig &cfg)
    : io_context_(io_context),
      config_(cfg),
      next_timer_id_(0),
      is_running_(false),
      cleanup_timer_(io_context),
      statistics_timer_(io_context) {
  initialize();
}

TimerManager::~TimerManager() { stop(); }

void TimerManager::start() {
  if (is_running_.exchange(true)) return;

  for (auto &bucket : buckets_) {
    bucket->start(io_context_);
  }

  if (config_.enable_cleanup) {
    start_cleanup_timer();
  }

  if (config_.enable_statistics) {
    start_statistics_timer();
  }
}

void TimerManager::stop() {
  if (!is_running_.exchange(false)) return;

  cleanup_timer_.cancel();
  statistics_timer_.cancel();

  for (auto &bucket : buckets_) {
    bucket->stop();
  }
}

TimerId TimerManager::schedule_at(TimerTimePoint when, TimerCallback callback, TimerPriority priority) {
  return schedule_impl(when, TimerDuration{0}, std::move(callback), false, priority);
}

TimerId TimerManager::schedule_after(TimerDuration delay, TimerCallback callback, TimerPriority priority) {
  return schedule_at(TimerClock::now() + delay, std::move(callback), priority);
}

TimerId TimerManager::schedule_every(TimerDuration interval, TimerCallback callback, TimerPriority priority) {
  return schedule_impl(TimerClock::now() + interval, interval, std::move(callback), true, priority);
}

bool TimerManager::cancel(TimerId timer_id) {
  std::size_t bucket_index = timer_id % config_.bucket_count;
  return buckets_[bucket_index]->cancel_task(timer_id);
}

TimerStatistics TimerManager::get_statistics() const {
  TimerStatistics total_stats;

  for (const auto &bucket : buckets_) {
  }

  return total_stats;
}

const TimerManagerConfig &TimerManager::config() const { return config_; }

// 更新配置
void TimerManager::update_config(const TimerManagerConfig &new_config) { config_ = new_config; }

void TimerManager::perform_cleanup() {
  for (auto &bucket : buckets_) {
  }
}

void TimerManager::report_statistics() {
  auto stats = get_statistics();
  std::cout << "=== Timer Manager Statistics ===" << std::endl;
  std::cout << "Total Scheduled: " << stats.total_scheduled.load() << std::endl;
  std::cout << "Total Executed: " << stats.total_executed.load() << std::endl;
  std::cout << "Total Cancelled: " << stats.total_cancelled.load() << std::endl;
  std::cout << "Total Failed: " << stats.total_failed.load() << std::endl;
  std::cout << "Current Pending: " << stats.current_pending.load() << std::endl;
  std::cout << "Average Execution Time: " << stats.average_execution_time().count() << "μs" << std::endl;
  std::cout << "================================" << std::endl;
}

void TimerManager::initialize() {
  buckets_.reserve(config_.bucket_count);
  for (std::size_t i = 0; i < config_.bucket_count; ++i) {
    buckets_.push_back(std::make_unique<TimerBucket>(i));
  }
}

TimerId TimerManager::schedule_impl(TimerTimePoint when, TimerDuration interval, TimerCallback callback,
                                        bool is_repeating, TimerPriority priority) {
  if (!is_running_.load()) {
    throw std::runtime_error("Timer manager is not running");
  }

  TimerId id = ++next_timer_id_;
  TimerTask task{when, std::move(callback), interval, is_repeating, priority, id, TimerClock::now()};

  std::size_t bucket_index = id % config_.bucket_count;
  if (buckets_[bucket_index]->add_task(task)) {
    statistics_.total_scheduled.fetch_add(1, std::memory_order_relaxed);
    return id;
  }

  throw std::runtime_error("Failed to schedule timer task");
}

void TimerManager::start_cleanup_timer() {
  cleanup_timer_.expires_after(config_.cleanup_interval);
  cleanup_timer_.async_wait([this](const std::error_code &ec) {
    if (!ec && is_running_.load()) {
      perform_cleanup();
      start_cleanup_timer();
    }
  });
}

void TimerManager::start_statistics_timer() {
  statistics_timer_.expires_after(config_.statistics_report_interval);
  statistics_timer_.async_wait([this](const std::error_code &ec) {
    if (!ec && is_running_.load()) {
      report_statistics();
      start_statistics_timer();
    }
  });
}

}  // namespace foxhttp::server
