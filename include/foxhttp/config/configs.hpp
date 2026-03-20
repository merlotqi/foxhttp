/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <chrono>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>

namespace foxhttp {

struct json_config {
  std::size_t max_size = 10 * 1024 * 1024;  // 10MB max JSON size
  std::size_t max_depth = 100;              // Max nesting depth
  bool strict_mode = true;                  // Strict JSON compliance
  bool allow_comments = false;              // Allow // and /* */ comments
  bool allow_trailing_commas = false;       // Allow trailing commas
  bool allow_duplicate_keys = true;         // Allow duplicate object keys
  bool validate_schema = false;             // Enable schema validation
  nlohmann::json schema;                    // JSON schema for validation
  std::string charset = "UTF-8";            // Default charset
};

struct multipart_config {
  std::size_t max_field_size = 10 * 1024 * 1024;   // 10MB max field size
  std::size_t max_file_size = 100 * 1024 * 1024;   // 100MB max file size
  std::size_t max_total_size = 500 * 1024 * 1024;  // 500MB max total size
  std::size_t memory_threshold = 1024 * 1024;      // 1MB threshold for temp files
  std::string temp_directory = "/tmp";             // Temporary directory
  bool auto_cleanup = true;                        // Auto cleanup temp files
  std::chrono::seconds temp_file_lifetime{3600};   // 1 hour temp file lifetime
  bool strict_mode = true;                         // Strict RFC compliance
  bool allow_empty_fields = false;                 // Allow empty field names
  std::vector<std::string> allowed_extensions;     // Allowed file extensions
  std::vector<std::string> allowed_content_types;  // Allowed content types
};

struct form_config {
  std::size_t max_field_size = 1024 * 1024;       // 1MB max field size
  std::size_t max_total_size = 10 * 1024 * 1024;  // 10MB max total size
  std::size_t max_fields = 1000;                  // Max number of fields
  bool strict_mode = true;                        // Strict RFC compliance
  bool allow_empty_values = true;                 // Allow empty field values
  bool support_arrays = true;                     // Support array notation (name[])
  std::string charset = "UTF-8";                  // Default charset
};

struct plain_text_config {
  std::size_t max_size = 10 * 1024 * 1024;         // 10MB max text size
  bool normalize_line_endings = true;              // Convert CRLF to LF
  bool trim_whitespace = false;                    // Trim leading/trailing whitespace
  bool validate_encoding = true;                   // Validate UTF-8 encoding
  std::string charset = "UTF-8";                   // Expected charset
  std::vector<std::string> allowed_content_types;  // Allowed content types
  bool strict_mode = false;                        // Strict content type checking
};

enum class load_balance_strategy {
  round_robin,
  least_connections,
  consistent_hash,
  random,
  weighted_round_robin
};

struct strand_pool_config {
  std::size_t initial_size = std::thread::hardware_concurrency();
  std::size_t min_size = 1;
  std::size_t max_size = 64;
  load_balance_strategy strategy = load_balance_strategy::round_robin;
  std::chrono::milliseconds health_check_interval{5000};
  std::chrono::milliseconds metrics_report_interval{10000};
  bool enable_auto_scaling = true;
  double load_threshold = 0.8;
  double idle_threshold = 0.2;
};

struct timer_manager_config {
  std::size_t bucket_count = 16;
  std::chrono::milliseconds cleanup_interval{60000};
  std::chrono::milliseconds statistics_report_interval{30000};
  bool enable_statistics = true;
  bool enable_cleanup = true;
};

struct session_limits {
  // Timeouts to mitigate slowloris/slow body attacks
  std::chrono::milliseconds handshake_timeout{5000};    // TLS handshake timeout
  std::chrono::milliseconds header_read_timeout{3000};  // time to read request line+headers
  std::chrono::milliseconds body_read_timeout{10000};   // time to make progress on body
  std::chrono::milliseconds idle_timeout{15000};        // no activity window

  // Size limits
  std::size_t max_header_bytes{64 * 1024};       // 64KB headers
  std::size_t max_body_bytes{50 * 1024 * 1024};  // 50MB body

  // Rate limiting (optional minimal expected progress)
  std::size_t min_read_progress_bytes{1024};  // bytes required per body window

  /// When true, honor HTTP keep-alive (Connection / HTTP version) after each response.
  bool enable_keep_alive{true};
  /// Close after this many completed responses on one connection; 0 means no limit.
  std::size_t max_requests_per_connection{1000};
};

struct websocket_limits {
  std::size_t max_message_bytes{8 * 1024 * 1024};  // 8MB
  std::size_t max_frame_bytes{2 * 1024 * 1024};    // 2MB
  std::chrono::milliseconds ping_interval{30000};  // 30s
  std::chrono::milliseconds pong_timeout{10000};   // 10s
  bool enable_compression{false};
};

}  // namespace foxhttp
