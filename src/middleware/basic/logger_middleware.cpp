#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <foxhttp/middleware/basic/logger_middleware.hpp>

namespace foxhttp {

logger_middleware::logger_middleware(const std::string &name, log_level level, log_format format,
                                     const std::string &log_file, bool enable_console)
    : name_(name), level_(level), format_(format), log_file_(log_file), enable_console_(enable_console) {
  setup_logger();
}

std::string logger_middleware::name() const { return name_; }

void logger_middleware::operator()(request_context &ctx, http::response<http::string_body> &res,
                                   std::function<void()> next) {
  auto start = std::chrono::steady_clock::now();
  auto request_id = generate_request_id();

  // Store request info in context
  ctx.set("request_id", request_id);
  ctx.set("logger_start_time", start);

  // Log request start
  log_request_start(ctx, request_id);

  try {
    next();

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Log request completion
    log_request_complete(ctx, res, request_id, duration, false);
  } catch (const std::exception &e) {
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Log request error
    log_request_error(ctx, res, request_id, duration, e.what());
    throw;  // Re-throw the exception
  }
}

void logger_middleware::operator()(request_context &ctx, http::response<http::string_body> &res,
                                   std::function<void()> next, async_middleware_callback callback) {
  auto start = std::chrono::steady_clock::now();
  auto request_id = generate_request_id();

  // Store request info in context
  ctx.set("request_id", request_id);
  ctx.set("logger_start_time", start);

  // Log request start
  log_request_start(ctx, request_id);

  // Create async callback wrapper
  auto async_callback = [this, &ctx, &res, request_id, start, callback](middleware_result result,
                                                                        const std::string &error_message) {
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    switch (result) {
      case middleware_result::continue_:
        log_request_complete(ctx, res, request_id, duration, true);
        break;
      case middleware_result::error:
        log_request_error(ctx, res, request_id, duration, error_message);
        break;
      case middleware_result::timeout:
        log_request_timeout(ctx, res, request_id, duration);
        break;
      case middleware_result::stop:
        log_request_stopped(ctx, res, request_id, duration);
        break;
    }

    callback(result, error_message);
  };

  next();
  async_callback(middleware_result::continue_, "");
}

// Configuration methods
void logger_middleware::set_log_level(log_level level) { level_ = level; }

void logger_middleware::set_log_format(log_format format) { format_ = format; }

void logger_middleware::set_log_file(const std::string &file) {
  log_file_ = file;
  setup_logger();
}

void logger_middleware::set_console_enabled(bool enabled) {
  enable_console_ = enabled;
  setup_logger();
}

void logger_middleware::setup_logger() {
  std::vector<spdlog::sink_ptr> sinks;

  // Console sink
  if (enable_console_) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(static_cast<spdlog::level::level_enum>(static_cast<int>(level_)));
    sinks.push_back(console_sink);
  }

  // File sink
  if (!log_file_.empty()) {
    try {
      auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_file_, 1024 * 1024 * 5,
                                                                              3);  // 5MB max, 3 files
      file_sink->set_level(static_cast<spdlog::level::level_enum>(static_cast<int>(level_)));
      sinks.push_back(file_sink);
    } catch (const spdlog::spdlog_ex &ex) {
      spdlog::error("Failed to create file sink: {}", ex.what());
    }
  }

  if (!sinks.empty()) {
    logger_ = std::make_shared<spdlog::logger>(name_, sinks.begin(), sinks.end());
    logger_->set_level(static_cast<spdlog::level::level_enum>(static_cast<int>(level_)));
    spdlog::register_logger(logger_);
  }
}

std::string logger_middleware::generate_request_id() {
  static std::atomic<uint64_t> counter{0};
  auto now = std::chrono::system_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
  return fmt::format("{}-{}", timestamp, ++counter);
}

void logger_middleware::log_request_start(const request_context &ctx, const std::string &request_id) {
  if (!logger_) return;

  switch (format_) {
    case log_format::simple:
      logger_->info("{} {} {} [{}]", ctx.method_string(), ctx.path(), ctx.header("User-Agent", "unknown"), request_id);
      break;

    case log_format::detailed:
      logger_->info("Request started - ID: {}, Method: {}, Path: {}, IP: {}, User-Agent: {}", request_id,
                    ctx.method_string(), ctx.path(), ctx.header("X-Forwarded-For", ctx.header("X-Real-IP", "unknown")),
                    ctx.header("User-Agent", "unknown"));
      break;

    case log_format::json:
      logger_->info(
          R"({{"event":"request_start","id":"{}","method":"{}","path":"{}","ip":"{}","user_agent":"{}","timestamp":"{}"}})",
          request_id, ctx.method_string(), ctx.path(),
          ctx.header("X-Forwarded-For", ctx.header("X-Real-IP", "unknown")), ctx.header("User-Agent", "unknown"),
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
              .count());
      break;

    case log_format::apache:
      logger_->info("{} - - [{}] \"{} {} HTTP/1.1\" - - \"{}\" \"{}\"",
                    ctx.header("X-Forwarded-For", ctx.header("X-Real-IP", "-")), get_apache_timestamp(),
                    ctx.method_string(), ctx.path(), ctx.header("Referer", "-"), ctx.header("User-Agent", "-"));
      break;
  }
}

void logger_middleware::log_request_complete(const request_context &ctx, const http::response<http::string_body> &res,
                                             const std::string &request_id, std::chrono::microseconds duration,
                                             bool async) {
  if (!logger_) return;

  auto status_code = static_cast<int>(res.result());
  auto duration_ms = duration.count() / 1000.0;

  switch (format_) {
    case log_format::simple:
      logger_->info("{} {} {} {} {}ms [{}]", ctx.method_string(), ctx.path(), status_code, res.body().size(),
                    duration_ms, request_id);
      break;

    case log_format::detailed:
      logger_->info(
          "Request completed - ID: {}, Method: {}, Path: {}, Status: {}, "
          "Size: {} bytes, Duration: {:.2f}ms, Async: {}",
          request_id, ctx.method_string(), ctx.path(), status_code, res.body().size(), duration_ms, async);
      break;

    case log_format::json:
      logger_->info(
          R"({{"event":"request_complete","id":"{}","method":"{}","path":"{}","status":{},"size":{},"duration_ms":{:.2f},"async":{},"timestamp":"{}"}})",
          request_id, ctx.method_string(), ctx.path(), status_code, res.body().size(), duration_ms, async,
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
              .count());
      break;

    case log_format::apache:
      logger_->info("{} - - [{}] \"{} {} HTTP/1.1\" {} {} \"{}\" \"{}\"",
                    ctx.header("X-Forwarded-For", ctx.header("X-Real-IP", "-")), get_apache_timestamp(),
                    ctx.method_string(), ctx.path(), status_code, res.body().size(), ctx.header("Referer", "-"),
                    ctx.header("User-Agent", "-"));
      break;
  }
}

void logger_middleware::log_request_error(const request_context &ctx, const http::response<http::string_body> &res,
                                          const std::string &request_id, std::chrono::microseconds duration,
                                          const std::string &error) {
  if (!logger_) return;

  auto status_code = static_cast<int>(res.result());
  auto duration_ms = duration.count() / 1000.0;

  switch (format_) {
    case log_format::simple:
      logger_->error("{} {} {} ERROR: {} [{}]", ctx.method_string(), ctx.path(), status_code, error, request_id);
      break;

    case log_format::detailed:
      logger_->error(
          "Request error - ID: {}, Method: {}, Path: {}, Status: {}, "
          "Error: {}, Duration: {:.2f}ms",
          request_id, ctx.method_string(), ctx.path(), status_code, error, duration_ms);
      break;

    case log_format::json:
      logger_->error(
          R"({{"event":"request_error","id":"{}","method":"{}","path":"{}","status":{},"error":"{}","duration_ms":{:.2f},"timestamp":"{}"}})",
          request_id, ctx.method_string(), ctx.path(), status_code, error, duration_ms,
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
              .count());
      break;

    case log_format::apache:
      logger_->error("{} - - [{}] \"{} {} HTTP/1.1\" {} {} \"{}\" \"{}\" ERROR: {}",
                     ctx.header("X-Forwarded-For", ctx.header("X-Real-IP", "-")), get_apache_timestamp(),
                     ctx.method_string(), ctx.path(), status_code, res.body().size(), ctx.header("Referer", "-"),
                     ctx.header("User-Agent", "-"), error);
      break;
  }
}

void logger_middleware::log_request_timeout(const request_context &ctx, const http::response<http::string_body> &res,
                                            const std::string &request_id, std::chrono::microseconds duration) {
  if (!logger_) return;

  auto duration_ms = duration.count() / 1000.0;
  logger_->warn("Request timeout - ID: {}, Method: {}, Path: {}, Duration: {:.2f}ms", request_id, ctx.method_string(),
                ctx.path(), duration_ms);
}

void logger_middleware::log_request_stopped(const request_context &ctx, const http::response<http::string_body> &res,
                                            const std::string &request_id, std::chrono::microseconds duration) {
  if (!logger_) return;

  auto duration_ms = duration.count() / 1000.0;
  logger_->info("Request stopped - ID: {}, Method: {}, Path: {}, Duration: {:.2f}ms", request_id, ctx.method_string(),
                ctx.path(), duration_ms);
}

std::string logger_middleware::get_apache_timestamp() {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto tm = *std::localtime(&time_t);

  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%d/%b/%Y:%H:%M:%S %z", &tm);
  return std::string(buffer);
}

}  // namespace foxhttp
