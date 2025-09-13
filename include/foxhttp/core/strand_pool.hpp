#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace foxhttp {

class AdvancedStrandPool
{
public:
    using StrandPtr = std::shared_ptr<boost::asio::strand<boost::asio::io_context::executor_type>>;

    using MetricsCallback = std::function<void(const std::string &, std::size_t)>;

    AdvancedStrandPool(boost::asio::io_context &io_context,
                       std::size_t initial_size = std::thread::hardware_concurrency(), std::size_t max_size = 64)
        : io_context_(io_context), max_size_(max_size), next_strand_(0)
    {
        resize(initial_size);
    }

    // 获取下一个 strand（智能负载均衡）
    StrandPtr getNextStrand()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // 简单的轮询算法
        auto &strand = strands_[next_strand_];
        ++next_strand_;
        if (next_strand_ >= strands_.size())
        {
            next_strand_ = 0;
        }

        // 更新使用统计
        updateUsageStatistics(next_strand_);

        return strand;
    }

    // 根据会话ID获取固定的 strand
    StrandPtr getStrandForSession(const std::string &session_id)
    {
        std::size_t hash = std::hash<std::string>{}(session_id);
        return getStrandByHash(hash);
    }

    StrandPtr getStrandByHash(std::size_t hash)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::size_t index = hash % strands_.size();
        updateUsageStatistics(index);
        return strands_[index];
    }

    // 动态调整池大小
    void resize(std::size_t new_size)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (new_size == 0 || new_size > max_size_)
        {
            throw std::runtime_error("Invalid strand pool size");
        }

        if (new_size == strands_.size())
        {
            return;
        }

        // 创建新的 strand 集合
        std::vector<StrandPtr> new_strands;
        for (std::size_t i = 0; i < new_size; ++i)
        {
            new_strands.push_back(std::make_shared<boost::asio::strand<boost::asio::io_context::executor_type>>(
                    io_context_.get_executor()));
        }

        strands_ = std::move(new_strands);
        usage_statistics_.resize(new_size);
        next_strand_ = 0;
    }

    // 设置指标回调
    void setMetricsCallback(MetricsCallback callback)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        metrics_callback_ = std::move(callback);
    }

    // 获取使用统计
    std::vector<std::size_t> getUsageStatistics() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return usage_statistics_;
    }

    // 获取池大小
    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return strands_.size();
    }

private:
    void updateUsageStatistics(std::size_t index)
    {
        if (index < usage_statistics_.size())
        {
            ++usage_statistics_[index];

            // 定期报告指标
            static std::size_t report_counter = 0;
            if (++report_counter % 1000 == 0 && metrics_callback_)
            {
                metrics_callback_("strand_usage", usage_statistics_[index]);
            }
        }
    }

    boost::asio::io_context &io_context_;
    std::vector<StrandPtr> strands_;
    std::vector<std::size_t> usage_statistics_;
    std::size_t max_size_;
    std::atomic<std::size_t> next_strand_;
    mutable std::mutex mutex_;
    MetricsCallback metrics_callback_;
};

}// namespace foxhttp