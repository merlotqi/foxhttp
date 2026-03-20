/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <boost/asio.hpp>
#include <chrono>
#include <foxhttp/server/request_context.hpp>
#include <functional>
#include <memory>

namespace foxhttp {

class middleware_chain;

struct middleware_stats {
  std::atomic<std::size_t> execution_count{0};
  std::atomic<std::chrono::microseconds> total_execution_time{std::chrono::microseconds{0}};
  std::atomic<std::size_t> error_count{0};
  std::atomic<std::size_t> timeout_count{0};

  middleware_stats() = default;
  middleware_stats(const middleware_stats &other);
  middleware_stats &operator=(const middleware_stats &other);

  void reset();
};

enum class middleware_priority : int {
  lowest = -1000,
  low = -100,
  normal = 0,
  high = 100,
  highest = 1000
};

enum class middleware_result {
  continue_,
  stop,
  error,
  timeout
};

using async_middleware_callback = std::function<void(middleware_result result, const std::string &error_message)>;
class middleware {
 public:
  virtual ~middleware() = default;

  virtual void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) = 0;
  virtual void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next,
                          async_middleware_callback callback);

  virtual middleware_priority priority() const;
  virtual std::string name() const;
  virtual bool should_execute(request_context &ctx) const;
  virtual std::chrono::milliseconds timeout() const;
  virtual middleware_stats &stats();
  virtual const middleware_stats &stats() const;

 protected:
  middleware_stats stats_;
};

template <middleware_priority Priority>
class priority_middleware : public middleware {
 public:
  middleware_priority priority() const override { return Priority; }
};

class conditional_middleware : public middleware {
 public:
  using condition_func = std::function<bool(request_context &)>;

  explicit conditional_middleware(std::shared_ptr<middleware> middleware, condition_func condition);
  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
  void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override;

  middleware_priority priority() const override;
  std::string name() const override;
  bool should_execute(request_context &ctx) const override;
  std::chrono::milliseconds timeout() const override;
  middleware_stats &stats() override;
  const middleware_stats &stats() const override;

 private:
  std::shared_ptr<middleware> mw_;
  condition_func condition_;
};

class middleware_builder {
 public:
  using sync_func = std::function<void(request_context &, http::response<http::string_body> &, std::function<void()>)>;
  using async_func = std::function<void(request_context &, http::response<http::string_body> &, std::function<void()>,
                                        async_middleware_callback)>;
  using condition_func = std::function<bool(request_context &)>;

  middleware_builder();
  ~middleware_builder() = default;

  middleware_builder &set_name(const std::string &name);
  middleware_builder &set_priority(middleware_priority priority);
  middleware_builder &set_timeout(std::chrono::milliseconds timeout);
  middleware_builder &set_sync_func(sync_func func);
  middleware_builder &set_async_func(async_func func);
  middleware_builder &set_condition(condition_func condition);
  std::shared_ptr<middleware> build();

 private:
  std::string name_;
  sync_func sync_func_;
  async_func async_func_;
  condition_func condition_;
  middleware_priority priority_;
  std::chrono::milliseconds timeout_;
};

}  // namespace foxhttp
