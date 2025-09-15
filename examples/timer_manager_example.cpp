/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * Timer Manager Example - 展示定时器管理器的使用方法
 */

#include <foxhttp/foxhttp.hpp>
#include <foxhttp/core/timer_manager.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <random>

using namespace foxhttp;

// 模拟工作负载
void simulate_workload(std::size_t duration_ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
}

// 测试任务
void test_task(const std::string& name, std::size_t task_id)
{
    std::cout << "Task " << name << " #" << task_id << " executed at " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now().time_since_epoch()).count()
              << "ms" << std::endl;
}

// 高优先级任务
void high_priority_task(const std::string& name)
{
    std::cout << "HIGH PRIORITY: " << name << " executed" << std::endl;
}

// 低优先级任务
void low_priority_task(const std::string& name)
{
    std::cout << "LOW PRIORITY: " << name << " executed" << std::endl;
}

// 重复任务
void repeating_task(std::size_t& counter)
{
    ++counter;
    std::cout << "Repeating task executed #" << counter << std::endl;
}

int main()
{
    try
    {
        std::cout << "=== Timer Manager Example ===" << std::endl;
        
        // 创建IO上下文
        boost::asio::io_context io_context;
        
        // 配置定时器管理器
        timer_manager::config config;
        config.bucket_count = 8;
        config.cleanup_interval = std::chrono::milliseconds(5000);
        config.statistics_report_interval = std::chrono::milliseconds(10000);
        config.enable_statistics = true;
        config.enable_cleanup = true;
        
        // 创建定时器管理器
        timer_manager manager(io_context, config);
        
        // 启动管理器
        manager.start();
        
        std::cout << "Timer manager started with " << config.bucket_count << " buckets" << std::endl;
        
        // 测试1: 基本定时器
        std::cout << "\n--- Test 1: Basic Timers ---" << std::endl;
        
        // 立即执行的任务
        auto timer1 = manager.schedule_after(std::chrono::milliseconds(100), 
                                           []() { test_task("Immediate", 1); });
        
        // 延迟执行的任务
        auto timer2 = manager.schedule_after(std::chrono::milliseconds(500), 
                                           []() { test_task("Delayed", 2); });
        
        // 测试2: 优先级任务
        std::cout << "\n--- Test 2: Priority Tasks ---" << std::endl;
        
        // 低优先级任务（延迟较短）
        auto timer3 = manager.schedule_after(std::chrono::milliseconds(200), 
                                           []() { low_priority_task("Low Priority Task"); },
                                           timer_priority::low);
        
        // 高优先级任务（延迟较长，但应该先执行）
        auto timer4 = manager.schedule_after(std::chrono::milliseconds(300), 
                                           []() { high_priority_task("High Priority Task"); },
                                           timer_priority::high);
        
        // 测试3: 重复任务
        std::cout << "\n--- Test 3: Repeating Tasks ---" << std::endl;
        
        std::size_t counter = 0;
        auto timer5 = manager.schedule_every(std::chrono::milliseconds(1000), 
                                           [&counter]() { repeating_task(counter); });
        
        // 测试4: 取消任务
        std::cout << "\n--- Test 4: Cancel Tasks ---" << std::endl;
        
        auto timer6 = manager.schedule_after(std::chrono::milliseconds(1500), 
                                           []() { test_task("Should be cancelled", 6); });
        
        // 立即取消任务
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        bool cancelled = manager.cancel(timer6);
        std::cout << "Task " << timer6 << " cancelled: " << (cancelled ? "Yes" : "No") << std::endl;
        
        // 测试5: 大量任务
        std::cout << "\n--- Test 5: Bulk Tasks ---" << std::endl;
        
        std::vector<timer_id_t> bulk_timers;
        for (int i = 0; i < 50; ++i)
        {
            auto timer = manager.schedule_after(
                std::chrono::milliseconds(2000 + i * 10), 
                [i]() { test_task("Bulk", i); }
            );
            bulk_timers.push_back(timer);
        }
        
        std::cout << "Scheduled " << bulk_timers.size() << " bulk tasks" << std::endl;
        
        // 测试6: 不同优先级的混合任务
        std::cout << "\n--- Test 6: Mixed Priority Tasks ---" << std::endl;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 3);
        
        for (int i = 0; i < 20; ++i)
        {
            auto priority = static_cast<timer_priority>(dis(gen));
            auto delay = std::chrono::milliseconds(3000 + i * 50);
            
            manager.schedule_after(delay, 
                [i, priority]() { 
                    test_task("Mixed Priority " + get_priority_name(priority), i); 
                }, priority);
        }
        
        // 运行IO上下文一段时间来执行任务
        std::cout << "\n--- Running IO Context ---" << std::endl;
        
        // 在单独线程中运行IO上下文
        std::thread io_thread([&io_context]() {
            io_context.run();
        });
        
        // 让主线程等待一段时间
        std::this_thread::sleep_for(std::chrono::seconds(8));
        
        // 打印最终统计信息
        print_timer_statistics(manager);
        
        // 停止管理器
        manager.stop();
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
