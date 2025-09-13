#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <unordered_map>

namespace foxhttp {

class AdvancedTimerManager
{
public:
    using TimerId = uint64_t;
    using TimerCallback = std::function<void()>;
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    enum class Priority
    {
        High,
        Normal,
        Low
    };

    struct TimerTask
    {
        TimePoint expiry;
        TimerCallback callback;
        std::chrono::milliseconds interval;
        bool repeating;
        Priority priority;
        TimerId id;

        bool operator>(const TimerTask &other) const
        {
            if (priority != other.priority)
            {
                return static_cast<int>(priority) > static_cast<int>(other.priority);
            }
            return expiry > other.expiry;
        }
    };

    explicit AdvancedTimerManager(boost::asio::io_context &io_context)
        : io_context_(io_context), next_timer_id_(0), master_timer_(io_context), stopped_(false)
    {
        startMasterTimer();
    }

    // 高级调度接口
    TimerId schedule(TimePoint when, TimerCallback callback, Priority priority = Priority::Normal)
    {
        return scheduleImpl(when, std::chrono::milliseconds(0), std::move(callback), false, priority);
    }

    TimerId scheduleAfter(std::chrono::milliseconds delay, TimerCallback callback, Priority priority = Priority::Normal)
    {
        return schedule(Clock::now() + delay, std::move(callback), priority);
    }

    TimerId scheduleEvery(std::chrono::milliseconds interval, TimerCallback callback,
                          Priority priority = Priority::Normal)
    {
        return scheduleImpl(Clock::now() + interval, interval, std::move(callback), true, priority);
    }

    // 取消定时器
    bool cancel(TimerId timer_id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return pending_tasks_.erase(timer_id) > 0;
    }

    // 优雅停止
    void stop()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_ = true;
        master_timer_.cancel();
        pending_tasks_.clear();
    }

    // 获取下一个即将触发的任务时间
    std::optional<TimePoint> getNextExpiry() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_tasks_.empty())
        {
            return std::nullopt;
        }

        TimePoint next_expiry = TimePoint::max();
        for (const auto &[id, task]: pending_tasks_)
        {
            if (task.expiry < next_expiry)
            {
                next_expiry = task.expiry;
            }
        }
        return next_expiry;
    }

    // 统计信息
    struct Statistics
    {
        std::size_t total_scheduled;
        std::size_t total_executed;
        std::size_t current_pending;
    };

    Statistics getStatistics() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return {total_scheduled_, total_executed_, pending_tasks_.size()};
    }

    ~AdvancedTimerManager()
    {
        stop();
    }

private:
    TimerId scheduleImpl(TimePoint when, std::chrono::milliseconds interval, TimerCallback callback, bool repeating,
                         Priority priority)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopped_)
        {
            throw std::runtime_error("TimerManager is stopped");
        }

        TimerId id = ++next_timer_id_;
        TimerTask task{when, std::move(callback), interval, repeating, priority, id};

        pending_tasks_.emplace(id, task);
        ++total_scheduled_;

        // 如果新任务是最早的，重新调度主定时器
        rescheduleMasterTimer();

        return id;
    }

    void startMasterTimer()
    {
        auto next_expiry = getNextExpiry();
        if (!next_expiry)
        {
            return;
        }

        auto now = Clock::now();
        if (*next_expiry <= now)
        {
            processExpiredTasks();
        }
        else
        {
            master_timer_.expires_at(*next_expiry);
            master_timer_.async_wait([this](const std::error_code &ec) {
                if (!ec && !stopped_)
                {
                    processExpiredTasks();
                    startMasterTimer();
                }
            });
        }
    }

    void processExpiredTasks()
    {
        std::vector<TimerTask> expired_tasks;
        auto now = Clock::now();

        {
            std::lock_guard<std::mutex> lock(mutex_);

            // 收集所有过期任务
            for (auto it = pending_tasks_.begin(); it != pending_tasks_.end();)
            {
                if (it->second.expiry <= now)
                {
                    expired_tasks.push_back(it->second);
                    it = pending_tasks_.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            // 按优先级排序
            std::sort(expired_tasks.begin(), expired_tasks.end(),
                      [](const TimerTask &a, const TimerTask &b) { return a.priority > b.priority; });
        }

        // 执行过期任务
        for (auto &task: expired_tasks)
        {
            try
            {
                if (task.callback)
                {
                    task.callback();
                }
                ++total_executed_;
            }
            catch (const std::exception &e)
            {
                std::cerr << "Timer task error: " << e.what() << std::endl;
            }

            // 如果是重复任务，重新调度
            if (task.repeating)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                TimerTask new_task = task;
                new_task.expiry = now + task.interval;
                pending_tasks_.emplace(new_task.id, new_task);
            }
        }
    }

    void rescheduleMasterTimer()
    {
        master_timer_.cancel();
        startMasterTimer();
    }

    boost::asio::io_context &io_context_;
    boost::asio::steady_timer master_timer_;
    std::unordered_map<TimerId, TimerTask> pending_tasks_;
    std::atomic<TimerId> next_timer_id_;
    std::atomic<bool> stopped_;
    mutable std::mutex mutex_;

    // 统计信息
    std::size_t total_scheduled_{0};
    std::size_t total_executed_{0};
};

}// namespace foxhttp