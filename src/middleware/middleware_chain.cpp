/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <algorithm>
#include <chrono>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <spdlog/spdlog.h>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace foxhttp {

middleware_chain::middleware_chain(boost::asio::io_context &io_context) : io_context_(&io_context) {}

void middleware_chain::register_run(std::shared_ptr<detail::pipeline_execution_state> state) {
  std::lock_guard<std::mutex> lock(runs_mutex_);
  active_runs_.push_back(std::move(state));
}

void middleware_chain::unregister_run(detail::pipeline_execution_state *raw) {
  std::lock_guard<std::mutex> lock(runs_mutex_);
  active_runs_.erase(std::remove_if(active_runs_.begin(), active_runs_.end(),
                                    [raw](const std::shared_ptr<detail::pipeline_execution_state> &p) {
                                      return p.get() == raw;
                                    }),
                     active_runs_.end());
}

bool middleware_chain::try_finish_run(const std::shared_ptr<detail::pipeline_execution_state> &state) {
  bool expected = false;
  if (!state->finished.compare_exchange_strong(expected, true)) {
    return false;
  }
  if (state->global_timeout_timer) {
    boost::system::error_code ec;
    state->global_timeout_timer->cancel(ec);
  }
  if (state->middleware_timeout_timer) {
    boost::system::error_code ec;
    state->middleware_timeout_timer->cancel(ec);
  }
  unregister_run(state.get());
  return true;
}

void middleware_chain::complete_async_run(const std::shared_ptr<detail::pipeline_execution_state> &state,
                                           completion_callback callback, middleware_result result,
                                           const std::string &message) {
  if (!try_finish_run(state)) {
    return;
  }
  if (callback) {
    callback(result, message);
  }
}

// middleware management
void middleware_chain::use(std::shared_ptr<middleware> mw) {
  std::lock_guard<std::mutex> lock(config_mutex_);
  mws_.push_back({std::move(mw)});
  if (auto_sort_by_priority_) {
    sort_middlewares_by_priority();
  }
}

void middleware_chain::use(std::initializer_list<std::shared_ptr<middleware>> mws) {
  std::lock_guard<std::mutex> lock(config_mutex_);
  for (auto &mw : mws) {
    mws_.push_back({std::move(mw)});
  }
  if (auto_sort_by_priority_) {
    sort_middlewares_by_priority();
  }
}

void middleware_chain::insert(std::size_t index, std::shared_ptr<middleware> mw) {
  std::lock_guard<std::mutex> lock(config_mutex_);
  if (index >= mws_.size()) {
    mws_.push_back({std::move(mw)});
  } else {
    mws_.insert(mws_.begin() + index, {std::move(mw)});
  }
}

void middleware_chain::insert_by_priority(std::shared_ptr<middleware> mw) {
  std::lock_guard<std::mutex> lock(config_mutex_);
  mws_.push_back({std::move(mw)});
  sort_middlewares_by_priority();
}

void middleware_chain::remove(const std::string &middleware_name) {
  std::lock_guard<std::mutex> lock(config_mutex_);
  mws_.erase(std::remove_if(mws_.begin(), mws_.end(),
                             [&middleware_name](const middleware_info &info) {
                               return info.mw_->name() == middleware_name;
                             }),
             mws_.end());
}

void middleware_chain::clear() {
  std::lock_guard<std::mutex> lock(config_mutex_);
  mws_.clear();
}

// Configuration
void middleware_chain::set_error_handler(error_handler handler) {
  std::lock_guard<std::mutex> lock(config_mutex_);
  error_handler_ = std::move(handler);
}

void middleware_chain::set_timeout_handler(timeout_handler handler) {
  std::lock_guard<std::mutex> lock(config_mutex_);
  timeout_handler_ = std::move(handler);
}

void middleware_chain::set_global_timeout(std::chrono::milliseconds timeout) { global_timeout_ = timeout; }

void middleware_chain::set_io_context(boost::asio::io_context &io_context) { io_context_ = &io_context; }

void middleware_chain::enable_statistics(bool enable) { statistics_enabled_ = enable; }

void middleware_chain::execute(request_context &ctx, http::response<http::string_body> &res) {
  std::shared_ptr<std::vector<std::shared_ptr<middleware>>> pipeline =
      std::make_shared<std::vector<std::shared_ptr<middleware>>>();
  error_handler eh;
  timeout_handler th;
  std::chrono::milliseconds global_to;
  bool stats = statistics_enabled_;
  {
    std::lock_guard<std::mutex> lock(config_mutex_);
    if (mws_.empty()) {
      return;
    }
    pipeline->reserve(mws_.size());
    for (const auto &info : mws_) {
      pipeline->push_back(info.mw_);
    }
    eh = error_handler_;
    th = timeout_handler_;
    global_to = global_timeout_;
  }

  auto state = std::make_shared<detail::pipeline_execution_state>();
  state->execution_start = std::chrono::steady_clock::now();
  register_run(state);

  try {
    execute_next_sync(pipeline, state, ctx, res, eh, th, global_to, stats);
  } catch (...) {
    try_finish_run(state);
    throw;
  }

  try_finish_run(state);
}

void middleware_chain::execute_async(request_context &ctx, http::response<http::string_body> &res,
                                     completion_callback callback) {
  std::shared_ptr<std::vector<std::shared_ptr<middleware>>> pipeline =
      std::make_shared<std::vector<std::shared_ptr<middleware>>>();
  error_handler eh;
  timeout_handler th;
  std::chrono::milliseconds global_to;
  bool stats = statistics_enabled_;
  boost::asio::io_context *io = io_context_;
  {
    std::lock_guard<std::mutex> lock(config_mutex_);
    if (mws_.empty()) {
      if (callback) {
        callback(middleware_result::continue_, "");
      }
      return;
    }
    pipeline->reserve(mws_.size());
    for (const auto &info : mws_) {
      pipeline->push_back(info.mw_);
    }
    eh = error_handler_;
    th = timeout_handler_;
    global_to = global_timeout_;
  }

  auto state = std::make_shared<detail::pipeline_execution_state>();
  state->execution_start = std::chrono::steady_clock::now();
  register_run(state);

  if (global_to > std::chrono::milliseconds{0} && io) {
    state->global_timeout_timer = std::make_shared<boost::asio::steady_timer>(*io, global_to);
    std::weak_ptr<detail::pipeline_execution_state> wstate = state;
    state->global_timeout_timer->async_wait(
        [this, wstate, &ctx, &res, callback, th](boost::system::error_code ec) {
          auto s = wstate.lock();
          if (!s || ec) {
            return;
          }
          if (!s->timed_out.exchange(true)) {
            handle_timeout_run(ctx, res, s, callback, th);
          }
        });
  }

  try {
    execute_next_async_step(pipeline, state, ctx, res, callback, eh, th, global_to, stats);
  } catch (const std::exception &e) {
    handle_error_run(ctx, res, e, state, callback, eh);
  } catch (...) {
    if (try_finish_run(state) && callback) {
      callback(middleware_result::error, "unknown error");
    }
    throw;
  }
}

void middleware_chain::cancel() {
  std::vector<std::shared_ptr<detail::pipeline_execution_state>> snapshot;
  {
    std::lock_guard<std::mutex> lock(runs_mutex_);
    snapshot = active_runs_;
  }
  for (const auto &s : snapshot) {
    s->cancelled.store(true);
    std::lock_guard<std::mutex> lk(s->mu);
    if (s->global_timeout_timer) {
      boost::system::error_code ec;
      s->global_timeout_timer->cancel(ec);
    }
    if (s->middleware_timeout_timer) {
      boost::system::error_code ec;
      s->middleware_timeout_timer->cancel(ec);
    }
  }
}

void middleware_chain::pause() { paused_.store(true, std::memory_order_release); }

void middleware_chain::resume() { paused_.store(false, std::memory_order_release); }

std::size_t middleware_chain::size() const {
  std::lock_guard<std::mutex> lock(config_mutex_);
  return mws_.size();
}

bool middleware_chain::empty() const {
  std::lock_guard<std::mutex> lock(config_mutex_);
  return mws_.empty();
}

bool middleware_chain::is_running() const {
  std::lock_guard<std::mutex> lock(runs_mutex_);
  return !active_runs_.empty();
}

std::vector<std::string> middleware_chain::get_middleware_names() const {
  std::lock_guard<std::mutex> lock(config_mutex_);
  std::vector<std::string> names;
  names.reserve(mws_.size());
  for (const auto &info : mws_) {
    names.push_back(info.mw_->name());
  }
  return names;
}

std::unordered_map<std::string, middleware_stats> middleware_chain::get_statistics() const {
  std::lock_guard<std::mutex> lock(config_mutex_);
  std::unordered_map<std::string, middleware_stats> stats;
  for (const auto &info : mws_) {
    stats[info.mw_->name()] = info.mw_->stats();
  }
  return stats;
}

void middleware_chain::reset_statistics() {
  std::lock_guard<std::mutex> lock(config_mutex_);
  for (auto &info : mws_) {
    info.mw_->stats().reset();
  }
}

void middleware_chain::print_statistics() const {
  auto stats = get_statistics();
  spdlog::info("=== middleware statistics ===");
  for (const auto &[name, stat] : stats) {
    spdlog::info("middleware: {}", name);
    spdlog::info("  executions: {}", stat.execution_count.load());
    spdlog::info("  total time (μs): {}", stat.total_execution_time.load().count());
    spdlog::info("  errors: {}", stat.error_count.load());
    spdlog::info("  timeouts: {}", stat.timeout_count.load());
  }
}

void middleware_chain::execute_next_sync(const std::shared_ptr<std::vector<std::shared_ptr<middleware>>> &pipeline,
                                          const std::shared_ptr<detail::pipeline_execution_state> &state,
                                          request_context &ctx, http::response<http::string_body> &res,
                                          const error_handler &eh, const timeout_handler &th,
                                          std::chrono::milliseconds global_timeout, bool statistics_enabled) {
  std::unique_lock<std::mutex> lk(state->mu);
  if (state->cancelled.load() || state->timed_out.load() || paused_.load(std::memory_order_acquire) ||
      state->current_index >= pipeline->size()) {
    lk.unlock();
    return;
  }

  if (global_timeout > std::chrono::milliseconds{0}) {
    auto now = std::chrono::steady_clock::now();
    if (now - state->execution_start > global_timeout) {
      state->timed_out.store(true);
      lk.unlock();
      if (th) {
        th(ctx, res);
      } else {
        res.result(http::status::gateway_timeout);
        res.set(http::field::content_type, "text/plain");
        res.body() = "Request timeout";
      }
      return;
    }
  }

  std::shared_ptr<middleware> mw = (*pipeline)[state->current_index];
  ++state->current_index;
  lk.unlock();

  if (!mw->should_execute(ctx)) {
    execute_next_sync(pipeline, state, ctx, res, eh, th, global_timeout, statistics_enabled);
    return;
  }

  std::chrono::steady_clock::time_point execution_start;
  if (statistics_enabled) {
    execution_start = std::chrono::steady_clock::now();
  }

  auto next = [this, pipeline, state, &ctx, &res, eh, th, global_timeout, statistics_enabled]() {
    {
      std::lock_guard<std::mutex> lk(state->mu);
      if (state->middleware_timeout_timer) {
        boost::system::error_code ec;
        state->middleware_timeout_timer->cancel(ec);
        state->middleware_timeout_timer.reset();
      }
    }
    execute_next_sync(pipeline, state, ctx, res, eh, th, global_timeout, statistics_enabled);
  };

  try {
    (*mw)(ctx, res, next);

    if (statistics_enabled) {
      auto execution_time = std::chrono::steady_clock::now() - execution_start;
      mw->stats().execution_count++;
      auto current_time = mw->stats().total_execution_time.load();
      auto new_time = current_time + std::chrono::duration_cast<std::chrono::microseconds>(execution_time);
      mw->stats().total_execution_time.store(new_time);
    }
  } catch (const std::exception &e) {
    if (statistics_enabled) {
      mw->stats().error_count++;
    }
    if (eh) {
      eh(ctx, res, e);
    } else {
      res.result(http::status::internal_server_error);
      res.set(http::field::content_type, "text/plain");
      res.body() = "middleware error: " + std::string(e.what());
    }
  }
}

void middleware_chain::execute_next_async_step(
    const std::shared_ptr<std::vector<std::shared_ptr<middleware>>> &pipeline,
    const std::shared_ptr<detail::pipeline_execution_state> &state, request_context &ctx,
    http::response<http::string_body> &res, completion_callback callback, const error_handler &eh,
    const timeout_handler &th, std::chrono::milliseconds global_timeout, bool statistics_enabled) {
  std::unique_lock<std::mutex> lk(state->mu);
  if (state->cancelled.load() || state->timed_out.load() || paused_.load(std::memory_order_acquire) ||
      state->current_index >= pipeline->size()) {
    lk.unlock();
    middleware_result r = state->cancelled.load()   ? middleware_result::stop
                          : state->timed_out.load() ? middleware_result::timeout
                                                    : middleware_result::continue_;
    complete_async_run(state, callback, r, "");
    return;
  }

  if (global_timeout > std::chrono::milliseconds{0} && !state->global_timeout_timer) {
    auto now = std::chrono::steady_clock::now();
    if (now - state->execution_start > global_timeout) {
      state->timed_out.store(true);
      lk.unlock();
      handle_timeout_run(ctx, res, state, callback, th);
      return;
    }
  }

  std::shared_ptr<middleware> mw = (*pipeline)[state->current_index];
  ++state->current_index;
  lk.unlock();

  if (!mw->should_execute(ctx)) {
    execute_next_async_step(pipeline, state, ctx, res, callback, eh, th, global_timeout, statistics_enabled);
    return;
  }

  std::chrono::steady_clock::time_point execution_start;
  if (statistics_enabled) {
    execution_start = std::chrono::steady_clock::now();
  }

  auto next = [this, pipeline, state, &ctx, &res, callback, eh, th, global_timeout, statistics_enabled]() {
    {
      std::lock_guard<std::mutex> lk(state->mu);
      if (state->middleware_timeout_timer) {
        boost::system::error_code ec;
        state->middleware_timeout_timer->cancel(ec);
        state->middleware_timeout_timer.reset();
      }
    }
    execute_next_async_step(pipeline, state, ctx, res, callback, eh, th, global_timeout, statistics_enabled);
  };

  auto async_callback = [this, pipeline, state, mw, execution_start, statistics_enabled, &ctx, &res, callback, eh, th,
                         global_timeout](middleware_result result, const std::string &error_message) {
    {
      std::lock_guard<std::mutex> lk(state->mu);
      if (state->middleware_timeout_timer) {
        boost::system::error_code ec;
        state->middleware_timeout_timer->cancel(ec);
        state->middleware_timeout_timer.reset();
      }
    }
    if (statistics_enabled) {
      auto execution_time = std::chrono::steady_clock::now() - execution_start;
      mw->stats().execution_count++;
      auto current_time = mw->stats().total_execution_time.load();
      auto new_time = current_time + std::chrono::duration_cast<std::chrono::microseconds>(execution_time);
      mw->stats().total_execution_time.store(new_time);

      if (result == middleware_result::error) {
        mw->stats().error_count++;
      } else if (result == middleware_result::timeout) {
        mw->stats().timeout_count++;
      }
    }

    handle_middleware_result_run(ctx, res, result, error_message, pipeline, state, callback, eh, th, global_timeout,
                                  statistics_enabled);
  };

  auto middleware_timeout = mw->timeout();
  if (middleware_timeout > std::chrono::milliseconds{0} && io_context_) {
    setup_middleware_timeout(mw, ctx, res, callback, state, th, statistics_enabled);
  }

  try {
    (*mw)(ctx, res, next, async_callback);
  } catch (const std::exception &e) {
    if (statistics_enabled) {
      mw->stats().error_count++;
    }
    handle_error_run(ctx, res, e, state, callback, eh);
  }
}

void middleware_chain::handle_error_run(request_context &ctx, http::response<http::string_body> &res,
                                         const std::exception &e,
                                         const std::shared_ptr<detail::pipeline_execution_state> &state,
                                         completion_callback callback, const error_handler &eh) {
  if (eh) {
    eh(ctx, res, e);
  } else {
    res.result(http::status::internal_server_error);
    res.set(http::field::content_type, "text/plain");
    res.body() = "middleware error: " + std::string(e.what());
  }

  complete_async_run(state, callback, middleware_result::error, e.what());
}

void middleware_chain::handle_timeout_run(request_context &ctx, http::response<http::string_body> &res,
                                           const std::shared_ptr<detail::pipeline_execution_state> &state,
                                           completion_callback callback, const timeout_handler &th) {
  if (th) {
    th(ctx, res);
  } else {
    res.result(http::status::gateway_timeout);
    res.set(http::field::content_type, "text/plain");
    res.body() = "Request timeout";
  }

  complete_async_run(state, callback, middleware_result::timeout, "Request timeout");
}

void middleware_chain::handle_middleware_result_run(
    request_context &ctx, http::response<http::string_body> &res, middleware_result result,
    const std::string &error_message, const std::shared_ptr<std::vector<std::shared_ptr<middleware>>> &pipeline,
    const std::shared_ptr<detail::pipeline_execution_state> &state, completion_callback callback,
    const error_handler &eh, const timeout_handler &th, std::chrono::milliseconds global_timeout,
    bool statistics_enabled) {
  switch (result) {
    case middleware_result::continue_:
      execute_next_async_step(pipeline, state, ctx, res, callback, eh, th, global_timeout, statistics_enabled);
      break;
    case middleware_result::stop:
      complete_async_run(state, callback, middleware_result::stop, "");
      break;
    case middleware_result::error:
      complete_async_run(state, callback, middleware_result::error, error_message);
      break;
    case middleware_result::timeout:
      complete_async_run(state, callback, middleware_result::timeout, error_message);
      break;
  }
}

void middleware_chain::sort_middlewares_by_priority() {
  std::sort(mws_.begin(), mws_.end(), [](const middleware_info &a, const middleware_info &b) {
    return static_cast<int>(a.mw_->priority()) < static_cast<int>(b.mw_->priority());
  });
}

void middleware_chain::setup_middleware_timeout(std::shared_ptr<middleware> mw, request_context &ctx,
                                                 http::response<http::string_body> &res, completion_callback callback,
                                                 const std::shared_ptr<detail::pipeline_execution_state> &state,
                                                 const timeout_handler &th, bool statistics_enabled) {
  if (!io_context_) {
    return;
  }

  auto timeout = mw->timeout();
  auto timer = std::make_shared<boost::asio::steady_timer>(*io_context_, timeout);
  {
    std::lock_guard<std::mutex> lk(state->mu);
    if (state->middleware_timeout_timer) {
      boost::system::error_code ec;
      state->middleware_timeout_timer->cancel(ec);
    }
    state->middleware_timeout_timer = timer;
  }

  std::weak_ptr<detail::pipeline_execution_state> wstate = state;
  timer->async_wait([this, wstate, mw, &ctx, &res, callback, th, statistics_enabled](boost::system::error_code ec) {
    auto s = wstate.lock();
    if (!s || ec) {
      return;
    }
    if (!s->timed_out.exchange(true)) {
      if (statistics_enabled) {
        mw->stats().timeout_count++;
      }
      handle_timeout_run(ctx, res, s, callback, th);
    }
  });
}

}  // namespace foxhttp
