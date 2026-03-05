#pragma once

#include <foxhttp/client/types.hpp>
#include <functional>
#include <future>
#include <memory>

namespace foxhttp {

template <typename T>
class promise {
 private:
  std::function<void(T)> then_callback_;
  std::function<void(const std::exception &)> catch_callback_;
  std::shared_ptr<std::promise<T>> promise_{std::make_shared<std::promise<T>>()};

 public:
  promise() = default;

  promise &then(std::function<void(T)> callback) {
    then_callback_ = std::move(callback);
    return *this;
  }
  promise &error_catch(std::function<void(const std::exception &)> callback) {
    catch_callback_ = std::move(callback);
    return *this;
  }

  void resolve(T value) {
    if (then_callback_) then_callback_(value);
    promise_->set_value(std::move(value));
  }

  void reject(const std::exception &error) {
    if (catch_callback_) catch_callback_(error);
    promise_->set_exception(std::make_exception_ptr(error));
  }

  std::future<T> get_future() { return promise_->get_future(); }
};

promise<response> fetch(const std::string &url);
promise<response> fetch(request req);

}  // namespace foxhttp
