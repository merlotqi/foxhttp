/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * Advanced Strand Pool Implementation
 */

#include <algorithm>
#include <foxhttp/server/strand_pool.hpp>
#include <iostream>

namespace foxhttp {

std::string format_duration(std::chrono::microseconds duration) {
  if (duration.count() < 1000) {
    return std::to_string(duration.count()) + "μs";
  } else if (duration.count() < 1000000) {
    return std::to_string(duration.count() / 1000) + "ms";
  } else {
    return std::to_string(duration.count() / 1000000) + "s";
  }
}

void print_strand_pool_statistics(const strand_pool &pool) {
  auto stats = pool.statistics();
  std::cout << "=== Strand Pool Statistics ===" << std::endl;
  std::cout << "Pool Size: " << pool.size() << std::endl;
  std::cout << "Strategy: ";

  switch (pool.config().strategy) {
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
      std::cout << "random";
      break;
    case load_balance_strategy::weighted_round_robin:
      std::cout << "Weighted Round Robin";
      break;
  }
  std::cout << std::endl;

  std::cout << "Strand Details:" << std::endl;
  for (std::size_t i = 0; i < stats.size(); ++i) {
    const auto &stat = stats[i];
    std::cout << "  Strand " << i << ":" << std::endl;
    std::cout << "    Total Requests: " << stat.total_requests.load() << std::endl;
    std::cout << "    Active Requests: " << stat.active_requests.load() << std::endl;
    std::cout << "    Completed Requests: " << stat.completed_requests.load() << std::endl;
    std::cout << "    Failed Requests: " << stat.failed_requests.load() << std::endl;
    std::cout << "    Load Factor: " << (stat.load_factor() * 100) << "%" << std::endl;
    std::cout << "    Avg Execution Time: " << format_duration(stat.average_execution_time()) << std::endl;
    std::cout << "    Healthy: " << (stat.is_healthy.load() ? "Yes" : "No") << std::endl;
  }
  std::cout << "==============================" << std::endl;
}

/* ------------------------------- strand_stats ------------------------------ */

strand_stats::strand_stats(const strand_stats &other) {
  total_requests.store(other.total_requests.load());
  active_requests.store(other.active_requests.load());
  completed_requests.store(other.completed_requests.load());
  failed_requests.store(other.failed_requests.load());
  total_execution_time.store(other.total_execution_time.load());
  last_used.store(other.last_used.load());
  is_healthy.store(other.is_healthy.load());
}

strand_stats &strand_stats::operator=(const strand_stats &other) {
  if (this != &other) {
    total_requests.store(other.total_requests.load());
    active_requests.store(other.active_requests.load());
    completed_requests.store(other.completed_requests.load());
    failed_requests.store(other.failed_requests.load());
    total_execution_time.store(other.total_execution_time.load());
    last_used.store(other.last_used.load());
    is_healthy.store(other.is_healthy.load());
  }
  return *this;
}

std::chrono::microseconds strand_stats::average_execution_time() const {
  auto completed = completed_requests.load();
  if (completed == 0) return std::chrono::microseconds{0};

  auto total = total_execution_time.load();
  return std::chrono::microseconds{total.count() / completed};
}

double strand_stats::load_factor() const {
  auto active = active_requests.load();
  auto completed = completed_requests.load();
  auto total = active + completed;

  if (total == 0) return 0.0;
  return static_cast<double>(active) / total;
}

/* --------------------------- strand_pool --------------------------- */

strand_pool::strand_pool(boost::asio::io_context &io_context, const strand_pool_config &config)
    : io_context_(io_context),
      config_(config),
      current_size_(0),
      round_robin_index_(0),
      random_generator_(std::random_device{}()),
      metrics_timer_(io_context),
      health_check_timer_(io_context),
      is_running_(false) {
  _initialize();
}

strand_pool::~strand_pool() { stop(); }

void strand_pool::start() {
  if (is_running_.exchange(true)) return;

  _start_metrics_reporting();
  _start_health_checking();
}

void strand_pool::stop() {
  if (!is_running_.exchange(false)) return;

  metrics_timer_.cancel();
  health_check_timer_.cancel();
}

strand_pool::strand_ptr strand_pool::next_strand() {
  switch (config_.strategy) {
    case load_balance_strategy::round_robin:
      return _round_robin_strand();
    case load_balance_strategy::least_connections:
      return _least_connections_strand();
    case load_balance_strategy::consistent_hash:
      return _consistent_hash_strand(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    case load_balance_strategy::random:
      return _random_strand();
    case load_balance_strategy::weighted_round_robin:
      return _weighted_round_robin_strand();
    default:
      return _round_robin_strand();
  }
}

strand_pool::strand_ptr strand_pool::strand_for_session(const std::string &session_id) {
  std::size_t hash = std::hash<std::string>{}(session_id);
  return _consistent_hash_strand(hash);
}

strand_pool::strand_ptr strand_pool::strand_by_hash(std::size_t hash) { return _consistent_hash_strand(hash); }

void strand_pool::resize(std::size_t new_size) {
  if (new_size < config_.min_size || new_size > config_.max_size) {
    throw std::invalid_argument("Invalid strand pool size");
  }

  std::unique_lock<std::shared_mutex> lock(strands_mutex_);

  if (new_size == current_size_) return;

  if (new_size > current_size_) {
    for (std::size_t i = current_size_; i < new_size; ++i) {
      _create_strand(i);
    }
  } else {
    std::vector<std::pair<std::size_t, double>> load_factors;
    for (std::size_t i = 0; i < current_size_; ++i) {
      load_factors.emplace_back(i, strands_stats_[i].load_factor());
    }

    std::sort(load_factors.begin(), load_factors.end(),
              [](const auto &a, const auto &b) { return a.second < b.second; });

    for (std::size_t i = 0; i < current_size_ - new_size; ++i) {
      std::size_t index_to_remove = load_factors[i].first;
      _remove_strand(index_to_remove);
    }
  }

  current_size_ = new_size;
  _update_consistent_hash_ring();
}

void strand_pool::set_load_balance_strategy(load_balance_strategy strategy) { config_.strategy = strategy; }

void strand_pool::set_metrics_callback(metrics_callback callback) {
  std::lock_guard<std::mutex> lock(metrics_mutex_);
  metrics_callback_ = std::move(callback);
}

void strand_pool::set_health_check_callback(health_check_callback callback) {
  std::lock_guard<std::mutex> lock(health_check_mutex_);
  health_check_callback_ = std::move(callback);
}

std::vector<strand_stats> strand_pool::statistics() const {
  std::shared_lock<std::shared_mutex> lock(strands_mutex_);
  return strands_stats_;
}

std::size_t strand_pool::size() const {
  std::shared_lock<std::shared_mutex> lock(strands_mutex_);
  return current_size_;
}

const strand_pool_config &strand_pool::config() const { return config_; }

void strand_pool::update_config(const strand_pool_config &new_config) {
  config_ = new_config;

  if (new_config.initial_size != current_size_) {
    resize(new_config.initial_size);
  }
}

void strand_pool::perform_health_check() { _perform_health_check_internal(); }

void strand_pool::report_metrics() { _report_metrics_internal(); }

void strand_pool::_initialize() {
  std::unique_lock<std::shared_mutex> lock(strands_mutex_);

  strands_.reserve(config_.max_size);
  strands_stats_.reserve(config_.max_size);

  for (std::size_t i = 0; i < config_.initial_size; ++i) {
    _create_strand(i);
  }

  current_size_ = config_.initial_size;
  _update_consistent_hash_ring();
}

void strand_pool::_create_strand(std::size_t index) {
  auto strand = std::make_shared<boost::asio::strand<executor_type>>(io_context_.get_executor());
  strands_.push_back(strand);
  strands_stats_.emplace_back();
}

void strand_pool::_remove_strand(std::size_t index) {
  if (index < strands_.size()) {
    strands_.erase(strands_.begin() + index);
    strands_stats_.erase(strands_stats_.begin() + index);
  }
}

strand_pool::strand_ptr strand_pool::_round_robin_strand() {
  std::shared_lock<std::shared_mutex> lock(strands_mutex_);
  if (strands_.empty()) return nullptr;

  std::size_t index = round_robin_index_.fetch_add(1, std::memory_order_relaxed) % current_size_;
  _update_strand_stats(index);
  return strands_[index];
}

strand_pool::strand_ptr strand_pool::_least_connections_strand() {
  std::shared_lock<std::shared_mutex> lock(strands_mutex_);
  if (strands_.empty()) return nullptr;

  std::size_t best_index = 0;
  std::size_t min_connections = strands_stats_[0].active_requests.load();

  for (std::size_t i = 1; i < current_size_; ++i) {
    std::size_t connections = strands_stats_[i].active_requests.load();
    if (connections < min_connections && strands_stats_[i].is_healthy.load()) {
      min_connections = connections;
      best_index = i;
    }
  }

  _update_strand_stats(best_index);
  return strands_[best_index];
}

strand_pool::strand_ptr strand_pool::_consistent_hash_strand(std::size_t hash) {
  std::shared_lock<std::shared_mutex> lock(strands_mutex_);
  if (strands_.empty()) return nullptr;

  auto it = std::lower_bound(hash_ring_.begin(), hash_ring_.end(), hash,
                             [](const auto &pair, std::size_t h) { return pair.first < h; });

  if (it == hash_ring_.end()) {
    it = hash_ring_.begin();
  }

  std::size_t index = it->second;
  _update_strand_stats(index);
  return strands_[index];
}

strand_pool::strand_ptr strand_pool::_random_strand() {
  std::shared_lock<std::shared_mutex> lock(strands_mutex_);
  if (strands_.empty()) return nullptr;

  std::uniform_int_distribution<std::size_t> dist(0, current_size_ - 1);
  std::size_t index = dist(random_generator_);
  _update_strand_stats(index);
  return strands_[index];
}

strand_pool::strand_ptr strand_pool::_weighted_round_robin_strand() {
  std::shared_lock<std::shared_mutex> lock(strands_mutex_);
  if (strands_.empty()) return nullptr;

  std::size_t total_weight = 0;
  for (std::size_t i = 0; i < current_size_; ++i) {
    if (strands_stats_[i].is_healthy.load()) {
      total_weight += (100 - static_cast<std::size_t>(strands_stats_[i].load_factor() * 100));
    }
  }

  if (total_weight == 0) return _round_robin_strand();

  std::uniform_int_distribution<std::size_t> dist(0, total_weight - 1);
  std::size_t random_weight = dist(random_generator_);

  std::size_t current_weight = 0;
  for (std::size_t i = 0; i < current_size_; ++i) {
    if (strands_stats_[i].is_healthy.load()) {
      current_weight += (100 - static_cast<std::size_t>(strands_stats_[i].load_factor() * 100));
      if (random_weight < current_weight) {
        _update_strand_stats(i);
        return strands_[i];
      }
    }
  }

  return _round_robin_strand();
}

void strand_pool::_update_strand_stats(std::size_t index) {
  if (index < strands_stats_.size()) {
    strands_stats_[index].total_requests.fetch_add(1, std::memory_order_relaxed);
    strands_stats_[index].active_requests.fetch_add(1, std::memory_order_relaxed);
    strands_stats_[index].last_used.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
  }
}

void strand_pool::_update_consistent_hash_ring() {
  hash_ring_.clear();
  hash_ring_.reserve(current_size_ * 100);

  for (std::size_t i = 0; i < current_size_; ++i) {
    for (std::size_t j = 0; j < 100; ++j) {
      std::string virtual_node = "strand_" + std::to_string(i) + "_" + std::to_string(j);
      std::size_t hash = std::hash<std::string>{}(virtual_node);
      hash_ring_.emplace_back(hash, i);
    }
  }

  std::sort(hash_ring_.begin(), hash_ring_.end());
}

void strand_pool::_start_metrics_reporting() {
  metrics_timer_.expires_after(config_.metrics_report_interval);
  metrics_timer_.async_wait([this](boost::system::error_code ec) {
    if (!ec && is_running_.load()) {
      _report_metrics_internal();
      _start_metrics_reporting();
    }
  });
}

void strand_pool::_start_health_checking() {
  health_check_timer_.expires_after(config_.health_check_interval);
  health_check_timer_.async_wait([this](boost::system::error_code ec) {
    if (!ec && is_running_.load()) {
      _perform_health_check_internal();
      _start_health_checking();
    }
  });
}

void strand_pool::_report_metrics_internal() {
  std::shared_lock<std::shared_mutex> lock(strands_mutex_);

  std::lock_guard<std::mutex> metrics_lock(metrics_mutex_);
  if (!metrics_callback_) return;

  for (std::size_t i = 0; i < current_size_; ++i) {
    const auto &stats = strands_stats_[i];

    metrics_callback_("strand_total_requests", std::to_string(i), static_cast<double>(stats.total_requests.load()));
    metrics_callback_("strand_active_requests", std::to_string(i), static_cast<double>(stats.active_requests.load()));
    metrics_callback_("strand_load_factor", std::to_string(i), stats.load_factor());
    metrics_callback_("strand_avg_execution_time", std::to_string(i),
                      static_cast<double>(stats.average_execution_time().count()));
  }
}

void strand_pool::_perform_health_check_internal() {
  std::shared_lock<std::shared_mutex> lock(strands_mutex_);

  std::lock_guard<std::mutex> health_lock(health_check_mutex_);
  if (!health_check_callback_) return;

  for (std::size_t i = 0; i < current_size_; ++i) {
    bool is_healthy = health_check_callback_(i);
    strands_stats_[i].is_healthy.store(is_healthy, std::memory_order_relaxed);
  }
}

}  // namespace foxhttp
