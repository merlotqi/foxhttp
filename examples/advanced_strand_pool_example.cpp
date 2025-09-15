/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * Advanced Strand Pool Example - 展示高级Strand池的使用方法
 */

#include <foxhttp/foxhttp.hpp>
#include <foxhttp/core/strand_pool.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <random>

using namespace foxhttp;

// 模拟工作负载
void simulateWorkload(std::size_t duration_ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
}

// 模拟异步任务
void simulateAsyncTask(strand_pool::strand_ptr  strand, std::size_t task_id, std::size_t duration_ms)
{
    boost::asio::post(*strand, [task_id, duration_ms]() {
        std::cout << "Task " << task_id << " started on strand" << std::endl;
        simulateWorkload(duration_ms);
        std::cout << "Task " << task_id << " completed" << std::endl;
    });
}

int main()
{
    try
    {
        std::cout << "=== Advanced Strand Pool Example ===" << std::endl;
        
        // 创建IO上下文
        boost::asio::io_context io_context;
        
        // 配置Strand池
        strand_pool_config config;
        config.initial_size = 4;
        config.min_size = 2;
        config.max_size = 8;
        config.strategy = load_balance_strategy::least_connections;
        config.health_check_interval = std::chrono::milliseconds(2000);
        config.metrics_report_interval = std::chrono::milliseconds(5000);
        config.enable_auto_scaling = true;
        config.load_threshold = 0.7;
        config.idle_threshold = 0.3;
        
        // 创建Strand池
        strand_pool strand_pool(io_context, config);
        
        // 设置指标回调
        strand_pool.set_metrics_callback([](const std::string& metric, const std::string& strand_id, double value) {
            std::cout << "Metric: " << metric << " Strand: " << strand_id << " Value: " << value << std::endl;
        });
        
        // 设置健康检查回调
        strand_pool.set_health_check_callback([](std::size_t strand_index) -> bool {
            // 模拟健康检查 - 90%的概率返回健康
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_real_distribution<> dis(0.0, 1.0);
            return dis(gen) > 0.1;
        });
        
        // 启动池
        strand_pool.start();
        
        std::cout << "Strand pool started with " << strand_pool.size() << " strands" << std::endl;
        
        // 测试不同的负载均衡策略
        std::vector<load_balance_strategy> strategies = {
            load_balance_strategy::round_robin,
            load_balance_strategy::least_connections,
            load_balance_strategy::consistent_hash,
            load_balance_strategy::random,
            load_balance_strategy::weighted_round_robin
        };
        
        for (auto strategy : strategies)
        {
            std::cout << "\n--- Testing Strategy: ";
            switch (strategy)
            {
                case load_balance_strategy::round_robin:
                    std::cout << "Round Robin";
                    break;
                case load_balance_strategy::least_connections:
                    std::cout << "Least Connections";
                    break;
                case load_balance_strategy::consistent_hash:
                    std::cout << "Consistent Hash";
                    break;
                case load_balance_strategy::random:
                    std::cout << "Random";
                    break;
                case load_balance_strategy::weighted_round_robin:
                    std::cout << "Weighted Round Robin";
                    break;
            }
            std::cout << " ---" << std::endl;
            
            strand_pool.set_load_balance_strategy(strategy);
            
            // 提交一些任务
            for (int i = 0; i < 10; ++i)
            {
                auto strand = strand_pool.next_strand();
                if (strand)
                {
                    simulateAsyncTask(strand, i, 100 + (i % 3) * 50); // 100-200ms的任务
                }
            }
            
            // 等待一段时间让任务执行
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        // 测试会话亲和性
        std::cout << "\n--- Testing session Affinity ---" << std::endl;
        std::vector<std::string> session_ids = {"user1", "user2", "user3", "user1", "user2", "user3"};
        
        for (const auto& session_id : session_ids)
        {
            auto strand = strand_pool.strand_for_session(session_id);
            if (strand)
            {
                std::cout << "session " << session_id << " assigned to strand" << std::endl;
            }
        }
        
        // 测试动态扩缩容
        std::cout << "\n--- Testing Dynamic Scaling ---" << std::endl;
        std::cout << "Current size: " << strand_pool.size() << std::endl;
        
        // 扩容
        strand_pool.resize(6);
        std::cout << "After expansion: " << strand_pool.size() << std::endl;
        
        // 缩容
        strand_pool.resize(3);
        std::cout << "After contraction: " << strand_pool.size() << std::endl;
        
        // 运行IO上下文一段时间来执行任务
        std::cout << "\n--- Running IO Context ---" << std::endl;
        
        // 在单独线程中运行IO上下文
        std::thread io_thread([&io_context]() {
            io_context.run();
        });
        
        // 让主线程等待一段时间
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // 停止池
        strand_pool.stop();
        io_context.stop();
        
        if (io_thread.joinable())
        {
            io_thread.join();
        }
        
        std::cout << "\nExample completed successfully!" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
