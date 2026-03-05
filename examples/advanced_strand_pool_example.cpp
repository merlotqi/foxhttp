/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <chrono>
#include <foxhttp/foxhttp.hpp>
#include <foxhttp/server/strand_pool.hpp>
#include <iostream>
#include <random>
#include <thread>
#include <vector>


using namespace foxhttp;

// Simulate workload
void simulateWorkload(std::size_t duration_ms) { std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms)); }

// Simulate async task
void simulateAsyncTask(strand_pool::strand_ptr strand, std::size_t task_id, std::size_t duration_ms) {
  boost::asio::post(*strand, [task_id, duration_ms]() {
    std::cout << "Task " << task_id << " started on strand" << std::endl;
    simulateWorkload(duration_ms);
    std::cout << "Task " << task_id << " completed" << std::endl;
  });
}

int main() {
  try {
    std::cout << "=== Advanced Strand Pool Example ===" << std::endl;
    // Create IO context
    boost::asio::io_context io_context;

    // Configure Strand pool
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

    // Create Strand pool
    strand_pool strand_pool(io_context, config);

    // Set metrics callback
    strand_pool.set_metrics_callback([](const std::string &metric, const std::string &strand_id, double value) {
      std::cout << "Metric: " << metric << " Strand: " << strand_id << " Value: " << value << std::endl;
    });

    // Set health check callback
    strand_pool.set_health_check_callback([](std::size_t strand_index) -> bool {
      // Simulate health check - 90% probability of being healthy
      static std::random_device rd;
      static std::mt19937 gen(rd());
      static std::uniform_real_distribution<> dis(0.0, 1.0);
      return dis(gen) > 0.1;
    });

    // Start the pool
    strand_pool.start();

    std::cout << "Strand pool started with " << strand_pool.size() << " strands" << std::endl;

    // Test different load balancing strategies
    std::vector<load_balance_strategy> strategies = {
        load_balance_strategy::round_robin, load_balance_strategy::least_connections,
        load_balance_strategy::consistent_hash, load_balance_strategy::random,
        load_balance_strategy::weighted_round_robin};

    for (auto strategy : strategies) {
      std::cout << "\n--- Testing Strategy: ";
      switch (strategy) {
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

      // Submit some tasks
      for (int i = 0; i < 10; ++i) {
        auto strand = strand_pool.next_strand();
        if (strand) {
          simulateAsyncTask(strand, i, 100 + (i % 3) * 50);  // 100-200ms tasks
        }
      }

      // Wait for tasks to execute
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Test session affinity
    std::cout << "\n--- Testing Session Affinity ---" << std::endl;
    std::vector<std::string> session_ids = {"user1", "user2", "user3", "user1", "user2", "user3"};

    for (const auto &session_id : session_ids) {
      auto strand = strand_pool.strand_for_session(session_id);
      if (strand) {
        std::cout << "Session " << session_id << " assigned to strand" << std::endl;
      }
    }

    // Test dynamic scaling
    std::cout << "\n--- Testing Dynamic Scaling ---" << std::endl;
    std::cout << "Current size: " << strand_pool.size() << std::endl;

    // Scale up
    strand_pool.resize(6);
    std::cout << "After expansion: " << strand_pool.size() << std::endl;

    // Scale down
    strand_pool.resize(3);
    std::cout << "After contraction: " << strand_pool.size() << std::endl;

    // Run IO context to execute tasks
    std::cout << "\n--- Running IO Context ---" << std::endl;

    // Run IO context in separate thread
    std::thread io_thread([&io_context]() { io_context.run(); });

    // Let main thread wait for a while
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Stop the pool
    strand_pool.stop();
    io_context.stop();

    if (io_thread.joinable()) {
      io_thread.join();
    }

    std::cout << "\nExample completed successfully!" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}