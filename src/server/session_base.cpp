#include <foxhttp/server/session_base.hpp>

namespace foxhttp {

session_base::session_base(const boost::asio::any_io_executor &executor, std::shared_ptr<middleware_chain> chain,
                           const session_limits &limits)
    : executor_(executor),
      chain_(std::move(chain)),
      limits_(limits),
      idle_timer_(executor_),
      header_timer_(executor_),
      body_timer_(executor_) {}

void session_base::set_limits(const session_limits &limits) { limits_ = limits; }

const session_limits &session_base::limits() const { return limits_; }

void session_base::arm_idle_timer() {
  idle_timer_.expires_after(limits_.idle_timeout);
  idle_timer_.async_wait([this](const boost::system::error_code &ec) {
    if (!ec) on_timeout_idle();
  });
}

void session_base::cancel_idle_timer() { idle_timer_.cancel(); }

void session_base::arm_header_timer() {
  header_timer_.expires_after(limits_.header_read_timeout);
  header_timer_.async_wait([this](const boost::system::error_code &ec) {
    if (!ec) on_timeout_header();
  });
}

void session_base::cancel_header_timer() { header_timer_.cancel(); }

void session_base::arm_body_timer() {
  body_timer_.expires_after(limits_.body_read_timeout);
  body_timer_.async_wait([this](const boost::system::error_code &ec) {
    if (!ec) on_timeout_body();
  });
}

void session_base::cancel_body_timer() { body_timer_.cancel(); }

void session_base::on_activity() {
  // Any network activity resets idle window
  cancel_idle_timer();
  arm_idle_timer();
}

void session_base::on_timeout_idle() {}

void session_base::on_timeout_header() {}

void session_base::on_timeout_body() {}

}  // namespace foxhttp
