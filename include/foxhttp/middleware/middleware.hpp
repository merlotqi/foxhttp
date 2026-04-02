#pragma once

#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <chrono>
#include <foxhttp/server/request_context.hpp>
#include <functional>
#include <memory>

namespace http = boost::beast::http;

namespace foxhttp::middleware {

using RequestContext = server::RequestContext;

class MiddlewareChain;

struct MiddlewareStats {
  std::atomic<std::size_t> execution_count{0};
  std::atomic<std::chrono::microseconds> total_execution_time{std::chrono::microseconds{0}};
  std::atomic<std::size_t> error_count{0};
  std::atomic<std::size_t> timeout_count{0};

  MiddlewareStats() = default;
  MiddlewareStats(const MiddlewareStats &other);
  MiddlewareStats &operator=(const MiddlewareStats &other);

  void reset();
};

enum class MiddlewarePriority : int {
  Lowest = -1000,
  Low = -100,
  Normal = 0,
  High = 100,
  Highest = 1000
};

enum class MiddlewareResult {
  Continue,
  Stop,
  Error,
  Timeout
};

using async_middleware_callback = std::function<void(MiddlewareResult result, const std::string &error_message)>;
class Middleware {
 public:
  virtual ~Middleware() = default;

  virtual void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next) = 0;
  virtual void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next,
                          async_middleware_callback callback);

  virtual MiddlewarePriority priority() const;
  virtual std::string name() const;
  virtual bool should_execute(RequestContext &ctx) const;
  virtual std::chrono::milliseconds timeout() const;
  virtual MiddlewareStats &stats();
  virtual const MiddlewareStats &stats() const;

 protected:
  MiddlewareStats stats_;
};

template <MiddlewarePriority Priority>
class PriorityMiddleware : public Middleware {
 public:
  MiddlewarePriority priority() const override { return Priority; }
};

class ConditionalMiddleware : public Middleware {
 public:
  using condition_func = std::function<bool(RequestContext &)>;

  explicit ConditionalMiddleware(std::shared_ptr<Middleware> middleware, condition_func condition);
  void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
  void operator()(RequestContext &ctx, http::response<http::string_body> &res, std::function<void()> next,
                  async_middleware_callback callback) override;

  MiddlewarePriority priority() const override;
  std::string name() const override;
  bool should_execute(RequestContext &ctx) const override;
  std::chrono::milliseconds timeout() const override;
  MiddlewareStats &stats() override;
  const MiddlewareStats &stats() const override;

 private:
  std::shared_ptr<Middleware> mw_;
  condition_func condition_;
};

class MiddlewareBuilder {
 public:
  using sync_func = std::function<void(RequestContext &, http::response<http::string_body> &, std::function<void()>)>;
  using async_func = std::function<void(RequestContext &, http::response<http::string_body> &, std::function<void()>,
                                        async_middleware_callback)>;
  using condition_func = std::function<bool(RequestContext &)>;

  MiddlewareBuilder();
  ~MiddlewareBuilder() = default;

  MiddlewareBuilder &set_name(const std::string &name);
  MiddlewareBuilder &set_priority(MiddlewarePriority priority);
  MiddlewareBuilder &set_timeout(std::chrono::milliseconds timeout);
  MiddlewareBuilder &set_sync_func(sync_func func);
  MiddlewareBuilder &set_async_func(async_func func);
  MiddlewareBuilder &set_condition(condition_func condition);
  std::shared_ptr<Middleware> build();

 private:
  std::string name_;
  sync_func sync_func_;
  async_func async_func_;
  condition_func condition_;
  MiddlewarePriority priority_;
  std::chrono::milliseconds timeout_;
};

}  // namespace foxhttp::middleware
