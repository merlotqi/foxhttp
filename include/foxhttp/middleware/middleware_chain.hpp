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

namespace foxhttp::middleware {

namespace detail {

// Per-request execution state (shared by async continuations); middleware list is snapshotted separately.
struct PipelineExecutionState {
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

class MiddlewareChain {
 public:
  using error_handler =
      std::function<void(RequestContext &, http::response<http::string_body> &, const std::exception &)>;
  using timeout_handler = std::function<void(RequestContext &, http::response<http::string_body> &)>;
  using completion_callback = std::function<void(MiddlewareResult result, const std::string &error_message)>;

  explicit MiddlewareChain(boost::asio::io_context &io_context);
  MiddlewareChain() = default;

  void use(std::shared_ptr<Middleware> mw);
  void use(std::initializer_list<std::shared_ptr<Middleware>> mws);
  void insert(std::size_t index, std::shared_ptr<Middleware> mw);
  void insert_by_priority(std::shared_ptr<Middleware> mw);
  void remove(const std::string &middleware_name);
  void clear();

  void set_error_handler(error_handler handler);
  void set_timeout_handler(timeout_handler handler);
  void set_global_timeout(std::chrono::milliseconds timeout);
  void set_io_context(boost::asio::io_context &io_context);
  void enable_statistics(bool enable = true);

  void execute(RequestContext &ctx, http::response<http::string_body> &res);
  void execute_async(RequestContext &ctx, http::response<http::string_body> &res,
                     completion_callback callback = nullptr);

  void cancel();
  void pause();
  void resume();

  std::size_t size() const;
  bool empty() const;
  bool is_running() const;
  std::vector<std::string> get_middleware_names() const;

  std::unordered_map<std::string, MiddlewareStats> get_statistics() const;
  void reset_statistics();
  void print_statistics() const;

 private:
  struct MiddlewareInfo {
    std::shared_ptr<Middleware> mw_;
  };

  void register_run(std::shared_ptr<detail::PipelineExecutionState> state);
  void unregister_run(detail::PipelineExecutionState *raw);

  bool try_finish_run(const std::shared_ptr<detail::PipelineExecutionState> &state);
  void complete_async_run(const std::shared_ptr<detail::PipelineExecutionState> &state, completion_callback callback,
                          MiddlewareResult result, const std::string &message);

  void execute_next_sync(const std::shared_ptr<std::vector<std::shared_ptr<Middleware>>> &pipeline,
                         const std::shared_ptr<detail::PipelineExecutionState> &state, RequestContext &ctx,
                         http::response<http::string_body> &res, const error_handler &eh, const timeout_handler &th,
                         std::chrono::milliseconds global_timeout, bool statistics_enabled);

  void execute_next_async_step(const std::shared_ptr<std::vector<std::shared_ptr<Middleware>>> &pipeline,
                               const std::shared_ptr<detail::PipelineExecutionState> &state, RequestContext &ctx,
                               http::response<http::string_body> &res, completion_callback callback,
                               const error_handler &eh, const timeout_handler &th,
                               std::chrono::milliseconds global_timeout, bool statistics_enabled);

  void handle_error_run(RequestContext &ctx, http::response<http::string_body> &res, const std::exception &e,
                        const std::shared_ptr<detail::PipelineExecutionState> &state, completion_callback callback,
                        const error_handler &eh);

  void handle_timeout_run(RequestContext &ctx, http::response<http::string_body> &res,
                          const std::shared_ptr<detail::PipelineExecutionState> &state, completion_callback callback,
                          const timeout_handler &th);

  void handle_middleware_result_run(RequestContext &ctx, http::response<http::string_body> &res,
                                    MiddlewareResult result, const std::string &error_message,
                                    const std::shared_ptr<std::vector<std::shared_ptr<Middleware>>> &pipeline,
                                    const std::shared_ptr<detail::PipelineExecutionState> &state,
                                    completion_callback callback, const error_handler &eh, const timeout_handler &th,
                                    std::chrono::milliseconds global_timeout, bool statistics_enabled);

  void sort_middlewares_by_priority();
  void setup_middleware_timeout(std::shared_ptr<Middleware> mw, RequestContext &ctx,
                                http::response<http::string_body> &res, completion_callback callback,
                                const std::shared_ptr<detail::PipelineExecutionState> &state,
                                const timeout_handler &th, bool statistics_enabled);

 private:
  mutable std::mutex config_mutex_;
  std::vector<MiddlewareInfo> mws_;
  mutable std::mutex runs_mutex_;
  std::vector<std::shared_ptr<detail::PipelineExecutionState>> active_runs_;

  bool statistics_enabled_ = true;
  bool auto_sort_by_priority_ = true;

  error_handler error_handler_;
  timeout_handler timeout_handler_;

  std::chrono::milliseconds global_timeout_{0};

  std::atomic<bool> paused_{false};

  boost::asio::io_context *io_context_ = nullptr;
};

}  // namespace foxhttp::middleware
