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
#include <foxhttp/middleware/middleware.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace foxhttp {

namespace detail {

// Per-request execution state (shared by async continuations); middleware list is snapshotted separately.
struct pipeline_execution_state {
  std::mutex mu;
  std::size_t current_index = 0;
  std::atomic<bool> timed_out{false};
  std::atomic<bool> cancelled{false};
  std::atomic<bool> finished{false};
  std::chrono::steady_clock::time_point execution_start{};
  std::shared_ptr<boost::asio::steady_timer> global_timeout_timer;
  std::shared_ptr<boost::asio::steady_timer> middleware_timeout_timer;
};

}  // namespace detail

class middleware_chain {
 public:
  using error_handler =
      std::function<void(request_context &, http::response<http::string_body> &, const std::exception &)>;
  using timeout_handler = std::function<void(request_context &, http::response<http::string_body> &)>;
  using completion_callback = std::function<void(middleware_result result, const std::string &error_message)>;

  explicit middleware_chain(boost::asio::io_context &io_context);
  middleware_chain() = default;

  void use(std::shared_ptr<middleware> mw);
  void use(std::initializer_list<std::shared_ptr<middleware>> mws);
  void insert(std::size_t index, std::shared_ptr<middleware> mw);
  void insert_by_priority(std::shared_ptr<middleware> mw);
  void remove(const std::string &middleware_name);
  void clear();

  void set_error_handler(error_handler handler);
  void set_timeout_handler(timeout_handler handler);
  void set_global_timeout(std::chrono::milliseconds timeout);
  void set_io_context(boost::asio::io_context &io_context);
  void enable_statistics(bool enable = true);

  void execute(request_context &ctx, http::response<http::string_body> &res);
  void execute_async(request_context &ctx, http::response<http::string_body> &res,
                     completion_callback callback = nullptr);

  void cancel();
  void pause();
  void resume();

  std::size_t size() const;
  bool empty() const;
  bool is_running() const;
  std::vector<std::string> get_middleware_names() const;

  std::unordered_map<std::string, middleware_stats> get_statistics() const;
  void reset_statistics();
  void print_statistics() const;

 private:
  struct middleware_info {
    std::shared_ptr<middleware> mw_;
  };

  void register_run(std::shared_ptr<detail::pipeline_execution_state> state);
  void unregister_run(detail::pipeline_execution_state *raw);

  bool try_finish_run(const std::shared_ptr<detail::pipeline_execution_state> &state);
  void complete_async_run(const std::shared_ptr<detail::pipeline_execution_state> &state,
                           completion_callback callback, middleware_result result, const std::string &message);

  void execute_next_sync(const std::shared_ptr<std::vector<std::shared_ptr<middleware>>> &pipeline,
                          const std::shared_ptr<detail::pipeline_execution_state> &state, request_context &ctx,
                          http::response<http::string_body> &res, const error_handler &eh, const timeout_handler &th,
                          std::chrono::milliseconds global_timeout, bool statistics_enabled);

  void execute_next_async_step(const std::shared_ptr<std::vector<std::shared_ptr<middleware>>> &pipeline,
                                const std::shared_ptr<detail::pipeline_execution_state> &state, request_context &ctx,
                                http::response<http::string_body> &res, completion_callback callback,
                                const error_handler &eh, const timeout_handler &th,
                                std::chrono::milliseconds global_timeout, bool statistics_enabled);

  void handle_error_run(request_context &ctx, http::response<http::string_body> &res, const std::exception &e,
                         const std::shared_ptr<detail::pipeline_execution_state> &state, completion_callback callback,
                         const error_handler &eh);

  void handle_timeout_run(request_context &ctx, http::response<http::string_body> &res,
                           const std::shared_ptr<detail::pipeline_execution_state> &state, completion_callback callback,
                           const timeout_handler &th);

  void handle_middleware_result_run(request_context &ctx, http::response<http::string_body> &res,
                                     middleware_result result, const std::string &error_message,
                                     const std::shared_ptr<std::vector<std::shared_ptr<middleware>>> &pipeline,
                                     const std::shared_ptr<detail::pipeline_execution_state> &state,
                                     completion_callback callback, const error_handler &eh, const timeout_handler &th,
                                     std::chrono::milliseconds global_timeout, bool statistics_enabled);

  void sort_middlewares_by_priority();
  void setup_middleware_timeout(std::shared_ptr<middleware> mw, request_context &ctx,
                                 http::response<http::string_body> &res, completion_callback callback,
                                 const std::shared_ptr<detail::pipeline_execution_state> &state,
                                 const timeout_handler &th, bool statistics_enabled);

 private:
  mutable std::mutex config_mutex_;
  std::vector<middleware_info> mws_;
  mutable std::mutex runs_mutex_;
  std::vector<std::shared_ptr<detail::pipeline_execution_state>> active_runs_;

  bool statistics_enabled_ = true;
  bool auto_sort_by_priority_ = true;

  error_handler error_handler_;
  timeout_handler timeout_handler_;

  std::chrono::milliseconds global_timeout_{0};

  std::atomic<bool> paused_{false};

  boost::asio::io_context *io_context_ = nullptr;
};

}  // namespace foxhttp
