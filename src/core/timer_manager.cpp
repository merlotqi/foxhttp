/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * Timer Manager Implementation
 */

#include <algorithm>
#include <foxhttp/core/timer_manager.hpp>
#include <iostream>

namespace foxhttp {

std::string format_duration(std::chrono::microseconds duration)
{
    if (duration.count() < 1000)
    {
        return std::to_string(duration.count()) + "μs";
    }
    else if (duration.count() < 1000000)
    {
        return std::to_string(duration.count() / 1000) + "ms";
    }
    else
    {
        return std::to_string(duration.count() / 1000000) + "s";
    }
}

std::string get_priority_name(timer_priority priority)
{
    switch (priority)
    {
        case timer_priority::low:
            return "Low";
        case timer_priority::normal:
            return "Normal";
        case timer_priority::high:
            return "High";
        case timer_priority::critical:
            return "Critical";
        default:
            return "Unknown";
    }
}

void print_timer_statistics(const timer_manager &manager)
{
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

bool timer_task::operator>(const timer_task &other) const
{
    if (priority != other.priority)
    {
        return static_cast<int>(priority) < static_cast<int>(other.priority);
    }
    return expiry_time > other.expiry_time;
}

bool timer_task::is_expired(const time_point_t &now) const
{
    return expiry_time <= now;
}

time_point_t timer_task::next_execution_time() const
{
    return expiry_time + interval;
}


/* ---------------------------- timer_statistics ---------------------------- */

timer_statistics::timer_statistics(const timer_statistics &rhs)
{
    total_scheduled.store(rhs.total_scheduled.load());
    total_executed.store(rhs.total_executed.load());
    total_cancelled.store(rhs.total_cancelled.load());
    total_failed.store(rhs.total_failed.load());
    current_pending.store(rhs.current_pending.load());
    total_execution_time.store(rhs.total_execution_time.load());
}

timer_statistics &timer_statistics::operator=(const timer_statistics &rhs)
{
    if (this == &rhs)
        return *this;

    total_scheduled.store(rhs.total_scheduled.load());
    total_executed.store(rhs.total_executed.load());
    total_cancelled.store(rhs.total_cancelled.load());
    total_failed.store(rhs.total_failed.load());
    current_pending.store(rhs.current_pending.load());
    total_execution_time.store(rhs.total_execution_time.load());

    return *this;
}

void timer_statistics::reset()
{
    total_scheduled = 0;
    total_executed = 0;
    total_cancelled = 0;
    total_failed = 0;
    current_pending = 0;
    total_execution_time = std::chrono::microseconds{0};
}

std::chrono::microseconds timer_statistics::average_execution_time() const
{
    auto executed = total_executed.load();
    if (executed == 0)
        return std::chrono::microseconds{0};

    auto total = total_execution_time.load();
    return std::chrono::microseconds{total.count() / executed};
}


/* ------------------------------ timer_bucket ------------------------------ */

timer_bucket::timer_bucket(std::size_t bucket_id) : bucket_id_(bucket_id), is_running_(false) {}

timer_bucket::~timer_bucket()
{
    stop();
}

void timer_bucket::start(boost::asio::io_context &io_context)
{
    if (is_running_.exchange(true))
        return;

    io_context_ = &io_context;
    timer_ = std::make_unique<boost::asio::steady_timer>(io_context);
    _start_timer();
}

void timer_bucket::stop()
{
    if (!is_running_.exchange(false))
        return;

    if (timer_)
    {
        timer_->cancel();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    pending_tasks_.clear();
}

bool timer_bucket::add_task(const timer_task &task)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!is_running_.load())
        return false;

    pending_tasks_.emplace(task.id, task);

    if (task.expiry_time < next_expiry_time_)
    {
        _reschedule_timer();
    }

    return true;
}

bool timer_bucket::cancel_task(timer_id_t timer_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return pending_tasks_.erase(timer_id) > 0;
}

std::optional<time_point_t> timer_bucket::get_next_expiry() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (pending_tasks_.empty())
    {
        return std::nullopt;
    }

    time_point_t next_expiry = time_point_t::max();
    for (const auto &[id, task]: pending_tasks_)
    {
        if (task.expiry_time < next_expiry)
        {
            next_expiry = task.expiry_time;
        }
    }

    return next_expiry;
}

std::size_t timer_bucket::get_pending_count() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return pending_tasks_.size();
}

void timer_bucket::_start_timer()
{
    auto next_expiry = get_next_expiry();
    if (!next_expiry)
    {
        return;
    }

    auto now = clock_t::now();
    if (*next_expiry <= now)
    {
        _process_expired_tasks();
    }
    else
    {
        timer_->expires_at(*next_expiry);
        timer_->async_wait([this](const std::error_code &ec) {
            if (!ec && is_running_.load())
            {
                _process_expired_tasks();
                _start_timer();
            }
        });
    }
}

void timer_bucket::_process_expired_tasks()
{
    std::vector<timer_task> expired_tasks;
    auto now = clock_t::now();

    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto it = pending_tasks_.begin(); it != pending_tasks_.end();)
        {
            if (it->second.is_expired(now))
            {
                expired_tasks.push_back(it->second);
                it = pending_tasks_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        std::sort(expired_tasks.begin(), expired_tasks.end(),
                  [](const timer_task &a, const timer_task &b) { return a.priority > b.priority; });
    }

    for (auto &task: expired_tasks)
    {
        auto start_time = clock_t::now();

        try
        {
            if (task.callback)
            {
                task.callback();
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Timer task error in bucket " << bucket_id_ << ": " << e.what() << std::endl;
        }

        auto execution_time = std::chrono::duration_cast<std::chrono::microseconds>(clock_t::now() - start_time);

        statistics_.total_executed.fetch_add(1, std::memory_order_relaxed);
        {
            auto old_value = statistics_.total_execution_time.load(std::memory_order_relaxed);
            while (!statistics_.total_execution_time.compare_exchange_weak(
                    old_value, std::chrono::microseconds{old_value.count() + execution_time.count()},
                    std::memory_order_relaxed, std::memory_order_relaxed))
            {
            }
        }

        if (task.is_repeating)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            timer_task new_task = task;
            new_task.expiry_time = task.next_execution_time();
            pending_tasks_.emplace(new_task.id, new_task);
        }
    }
}

void timer_bucket::_reschedule_timer()
{
    if (timer_)
    {
        timer_->cancel();
    }
    _start_timer();
}


/* ------------------------------ timer_manager ----------------------------- */

timer_manager::timer_manager(boost::asio::io_context &io_context, const timer_manager_config &cfg)
    : io_context_(io_context), config_(cfg), next_timer_id_(0), is_running_(false), cleanup_timer_(io_context),
      statistics_timer_(io_context)
{
    _initialize();
}

timer_manager::~timer_manager()
{
    stop();
}

void timer_manager::start()
{
    if (is_running_.exchange(true))
        return;

    for (auto &bucket: buckets_)
    {
        bucket->start(io_context_);
    }

    if (config_.enable_cleanup)
    {
        _start_cleanup_timer();
    }

    if (config_.enable_statistics)
    {
        _start_statistics_timer();
    }
}

void timer_manager::stop()
{
    if (!is_running_.exchange(false))
        return;

    cleanup_timer_.cancel();
    statistics_timer_.cancel();

    for (auto &bucket: buckets_)
    {
        bucket->stop();
    }
}

timer_id_t timer_manager::schedule_at(time_point_t when, timer_callback_t callback, timer_priority priority)
{
    return _schedule_impl(when, duration_t{0}, std::move(callback), false, priority);
}

timer_id_t timer_manager::schedule_after(duration_t delay, timer_callback_t callback, timer_priority priority)
{
    return schedule_at(clock_t::now() + delay, std::move(callback), priority);
}

timer_id_t timer_manager::schedule_every(duration_t interval, timer_callback_t callback, timer_priority priority)
{
    return _schedule_impl(clock_t::now() + interval, interval, std::move(callback), true, priority);
}

bool timer_manager::cancel(timer_id_t timer_id)
{
    std::size_t bucket_index = timer_id % config_.bucket_count;
    return buckets_[bucket_index]->cancel_task(timer_id);
}

timer_statistics timer_manager::get_statistics() const
{
    timer_statistics total_stats;

    for (const auto &bucket: buckets_)
    {
    }

    return total_stats;
}

const timer_manager_config &timer_manager::config() const
{
    return config_;
}

// 更新配置
void timer_manager::update_config(const timer_manager_config &new_config)
{
    config_ = new_config;
}

void timer_manager::perform_cleanup()
{
    for (auto &bucket: buckets_)
    {
    }
}

void timer_manager::report_statistics()
{
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

void timer_manager::_initialize()
{
    buckets_.reserve(config_.bucket_count);
    for (std::size_t i = 0; i < config_.bucket_count; ++i)
    {
        buckets_.push_back(std::make_unique<timer_bucket>(i));
    }
}

timer_id_t timer_manager::_schedule_impl(time_point_t when, duration_t interval, timer_callback_t callback,
                                         bool is_repeating, timer_priority priority)
{
    if (!is_running_.load())
    {
        throw std::runtime_error("Timer manager is not running");
    }

    timer_id_t id = ++next_timer_id_;
    timer_task task{when, std::move(callback), interval, is_repeating, priority, id, clock_t::now()};

    std::size_t bucket_index = id % config_.bucket_count;
    if (buckets_[bucket_index]->add_task(task))
    {
        statistics_.total_scheduled.fetch_add(1, std::memory_order_relaxed);
        return id;
    }

    throw std::runtime_error("Failed to schedule timer task");
}

void timer_manager::_start_cleanup_timer()
{
    cleanup_timer_.expires_after(config_.cleanup_interval);
    cleanup_timer_.async_wait([this](const std::error_code &ec) {
        if (!ec && is_running_.load())
        {
            perform_cleanup();
            _start_cleanup_timer();
        }
    });
}

void timer_manager::_start_statistics_timer()
{
    statistics_timer_.expires_after(config_.statistics_report_interval);
    statistics_timer_.async_wait([this](const std::error_code &ec) {
        if (!ec && is_running_.load())
        {
            report_statistics();
            _start_statistics_timer();
        }
    });
}

}// namespace foxhttp
