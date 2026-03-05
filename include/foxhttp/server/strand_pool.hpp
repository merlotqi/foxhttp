/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <foxhttp/config/configs.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <vector>

namespace foxhttp {

using strand_pool_config = ::foxhttp::strand_pool_config;

struct strand_stats {
  std::atomic<std::size_t> total_requests{0};
  std::atomic<std::size_t> active_requests{0};
  std::atomic<std::size_t> completed_requests{0};
  std::atomic<std::size_t> failed_requests{0};
  std::atomic<std::chrono::microseconds> total_execution_time{std::chrono::microseconds{0}};
  std::atomic<std::chrono::steady_clock::time_point> last_used{std::chrono::steady_clock::now()};
  std::atomic<bool> is_healthy{true};

  strand_stats() = default;
  strand_stats(const strand_stats &other);
  strand_stats &operator=(const strand_stats &other);
  std::chrono::microseconds average_execution_time() const;
  double load_factor() const;
};

class strand_pool {
 public:
  using metrics_callback = std::function<void(const std::string &, const std::string &, double)>;
  using health_check_callback = std::function<bool(std::size_t strand_index)>;
  using executor_type = boost::asio::io_context::executor_type;
  using strand_ptr = std::shared_ptr<boost::asio::strand<executor_type>>;

  explicit strand_pool(boost::asio::io_context &io_context, const strand_pool_config &config = {});
  ~strand_pool();

  void start();
  void stop();

  strand_ptr next_strand();
  strand_ptr strand_for_session(const std::string &session_id);
  strand_ptr strand_by_hash(std::size_t hash);

  void resize(std::size_t new_size);
  void set_load_balance_strategy(load_balance_strategy strategy);
  void set_metrics_callback(metrics_callback callback);
  void set_health_check_callback(health_check_callback callback);
  std::vector<strand_stats> statistics() const;

  std::size_t size() const;

  const strand_pool_config &config() const;
  void update_config(const strand_pool_config &new_config);

  void perform_health_check();
  void report_metrics();

 private:
  void _initialize();
  void _create_strand(std::size_t index);
  void _remove_strand(std::size_t index);

  strand_ptr _round_robin_strand();
  strand_ptr _least_connections_strand();
  strand_ptr _consistent_hash_strand(std::size_t hash);
  strand_ptr _random_strand();
  strand_ptr _weighted_round_robin_strand();

  void _update_strand_stats(std::size_t index);
  void _update_consistent_hash_ring();
  void _start_metrics_reporting();
  void _start_health_checking();
  void _report_metrics_internal();
  void _perform_health_check_internal();

 private:
  boost::asio::io_context &io_context_;
  strand_pool_config config_;

  mutable std::shared_mutex strands_mutex_;
  std::vector<strand_ptr> strands_;
  std::vector<strand_stats> strands_stats_;
  std::size_t current_size_;

  std::atomic<std::size_t> round_robin_index_{0};
  std::mt19937 random_generator_;
  std::vector<std::pair<std::size_t, std::size_t>> hash_ring_;  // <hash, strand_index>

  boost::asio::steady_timer metrics_timer_;
  boost::asio::steady_timer health_check_timer_;

  mutable std::mutex metrics_mutex_;
  metrics_callback metrics_callback_;

  mutable std::mutex health_check_mutex_;
  health_check_callback health_check_callback_;

  std::atomic<bool> is_running_;
};

}  // namespace foxhttp
