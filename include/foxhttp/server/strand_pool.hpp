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

namespace foxhttp::server {

using StrandPoolConfig = config::StrandPoolConfig;
using LoadBalanceStrategy = config::LoadBalanceStrategy;

struct StrandStats {
  std::atomic<std::size_t> total_requests{0};
  std::atomic<std::size_t> active_requests{0};
  std::atomic<std::size_t> completed_requests{0};
  std::atomic<std::size_t> failed_requests{0};
  std::atomic<std::chrono::microseconds> total_execution_time{std::chrono::microseconds{0}};
  std::atomic<std::chrono::steady_clock::time_point> last_used{std::chrono::steady_clock::now()};
  std::atomic<bool> is_healthy{true};

  StrandStats() = default;
  StrandStats(const StrandStats &other);
  StrandStats &operator=(const StrandStats &other);
  std::chrono::microseconds average_execution_time() const;
  double load_factor() const;
};

class StrandPool {
 public:
  using MetricsCallback = std::function<void(const std::string &, const std::string &, double)>;
  using HealthCheckCallback = std::function<bool(std::size_t strand_index)>;
  using executor_type = boost::asio::io_context::executor_type;
  using strand_ptr = std::shared_ptr<boost::asio::strand<executor_type>>;

  explicit StrandPool(boost::asio::io_context &io_context, const StrandPoolConfig &config = {});
  ~StrandPool();

  void start();
  void stop();

  strand_ptr next_strand();
  strand_ptr strand_for_session(const std::string &session_id);
  strand_ptr strand_by_hash(std::size_t hash);

  void resize(std::size_t new_size);
  void set_load_balance_strategy(LoadBalanceStrategy strategy);
  void set_metrics_callback(MetricsCallback callback);
  void set_health_check_callback(HealthCheckCallback callback);
  std::vector<StrandStats> statistics() const;

  std::size_t size() const;

  const StrandPoolConfig &config() const;
  void update_config(const StrandPoolConfig &new_config);

  void perform_health_check();
  void report_metrics();

 private:
  void initialize();
  void create_strand(std::size_t index);
  void remove_strand(std::size_t index);

  strand_ptr round_robin_strand();
  strand_ptr least_connections_strand();
  strand_ptr consistent_hash_strand(std::size_t hash);
  strand_ptr random_strand();
  strand_ptr weightedround_robin_strand();

  void update_strand_stats(std::size_t index);
  void update_consistent_hash_ring();
  void start_metrics_reporting();
  void start_health_checking();
  void report_metrics_internal();
  void perform_health_check_internal();

 private:
  boost::asio::io_context &io_context_;
  StrandPoolConfig config_;

  mutable std::shared_mutex strands_mutex_;
  std::vector<strand_ptr> strands_;
  std::vector<StrandStats> strands_stats_;
  std::size_t current_size_;

  std::atomic<std::size_t> round_robin_index_{0};
  std::mt19937 random_generator_;
  std::vector<std::pair<std::size_t, std::size_t>> hash_ring_;  // <hash, strand_index>

  boost::asio::steady_timer metrics_timer_;
  boost::asio::steady_timer health_check_timer_;

  mutable std::mutex metrics_mutex_;
  MetricsCallback metrics_callback_;

  mutable std::mutex health_check_mutex_;
  HealthCheckCallback health_check_callback_;

  std::atomic<bool> is_running_;
};

}  // namespace foxhttp::server
