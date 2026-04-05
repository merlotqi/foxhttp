#include <foxhttp/server/session_base.hpp>

namespace foxhttp::server {

SessionBase::SessionBase(const boost::asio::any_io_executor &executor, std::shared_ptr<MiddlewareChain> chain,
                           const SessionLimits &limits)
    : executor_(executor),
      chain_(std::move(chain)),
      limits_(limits),
      idle_timer_(executor_),
      header_timer_(executor_),
      body_timer_(executor_) {}

void SessionBase::set_limits(const SessionLimits &limits) { limits_ = limits; }

const SessionLimits &SessionBase::limits() const { return limits_; }

void SessionBase::arm_idle_timer() {
  idle_timer_.expires_after(limits_.idle_timeout);
  idle_timer_.async_wait([this](const boost::system::error_code &ec) {
    if (!ec) on_timeout_idle();
  });
}

void SessionBase::cancel_idle_timer() { idle_timer_.cancel(); }

void SessionBase::arm_header_timer() {
  header_timer_.expires_after(limits_.header_read_timeout);
  header_timer_.async_wait([this](const boost::system::error_code &ec) {
    if (!ec) on_timeout_header();
  });
}

void SessionBase::cancel_header_timer() { header_timer_.cancel(); }

void SessionBase::arm_body_timer() {
  body_timer_.expires_after(limits_.body_read_timeout);
  body_timer_.async_wait([this](const boost::system::error_code &ec) {
    if (!ec) on_timeout_body();
  });
}

void SessionBase::cancel_body_timer() { body_timer_.cancel(); }

void SessionBase::on_activity() {
  // Any network activity resets idle window
  cancel_idle_timer();
  arm_idle_timer();
}

void SessionBase::on_timeout_idle() {}

void SessionBase::on_timeout_header() {}

void SessionBase::on_timeout_body() {}

void SessionBase::set_error_callback(error_callback cb) { error_cb_ = std::move(cb); }

void SessionBase::notify_error(const std::exception_ptr &eptr) {
  if (error_cb_) {
    error_cb_(eptr);
  }
}

}  // namespace foxhttp::server
