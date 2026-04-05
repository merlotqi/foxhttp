#pragma once

#include <boost/beast/http.hpp>
#include <chrono>
#include <cstddef>

namespace foxhttp::core {

/* -------------------------------------------------------------------------- */
/*                             Default Size Limits                            */
/* -------------------------------------------------------------------------- */

/// Default maximum body size for JSON parser (10 MB)
constexpr std::size_t kDefaultMaxBodySize = 10 * 1024 * 1024;

/// Default maximum JSON nesting depth
constexpr std::size_t kDefaultMaxJsonDepth = 100;

/// Default maximum field size for multipart parser (10 MB)
constexpr std::size_t kDefaultMaxFieldSize = 10 * 1024 * 1024;

/// Default maximum file size for multipart parser (100 MB)
constexpr std::size_t kDefaultMaxFileSize = 100 * 1024 * 1024;

/// Default maximum total size for multipart parser (500 MB)
constexpr std::size_t kDefaultMaxTotalSize = 500 * 1024 * 1024;

/// Default memory threshold for multipart parser (1 MB)
constexpr std::size_t kDefaultMemoryThreshold = 1024 * 1024;

/// Default maximum number of form fields
constexpr std::size_t kDefaultMaxFields = 1000;

/* -------------------------------------------------------------------------- */
/*                              Default Timeouts                              */
/* -------------------------------------------------------------------------- */

/// Default I/O timeout for HTTP client (30 seconds)
constexpr auto kDefaultIoTimeout = std::chrono::seconds(30);

/// Default SSL handshake timeout (30 seconds)
constexpr auto kDefaultHandshakeTimeout = std::chrono::seconds(30);

/// Default idle timeout for sessions (60 seconds)
constexpr auto kDefaultIdleTimeout = std::chrono::seconds(60);

/// Default header read timeout (30 seconds)
constexpr auto kDefaultHeaderTimeout = std::chrono::seconds(30);

/// Default body read timeout (30 seconds)
constexpr auto kDefaultBodyTimeout = std::chrono::seconds(30);

/// Default global middleware timeout (0 = no timeout)
constexpr auto kDefaultGlobalTimeout = std::chrono::milliseconds(0);

/* -------------------------------------------------------------------------- */
/*                          Connection Pool Defaults                          */
/* -------------------------------------------------------------------------- */

/// Default maximum Connection pool size
constexpr std::size_t kDefaultMaxPoolSize = 100;

/// Default Connection idle timeout (60 seconds)
constexpr auto kDefaultConnectionIdleTimeout = std::chrono::seconds(60);

/// Default Connection cleanup interval (10 seconds)
constexpr auto kDefaultCleanupInterval = std::chrono::seconds(10);

/* -------------------------------------------------------------------------- */
/*                               Server Defaults                              */
/* -------------------------------------------------------------------------- */

/// Default maximum requests per Connection (0 = unlimited)
constexpr std::size_t kDefaultMaxRequestsPerConnection = 0;

/// Default io_context pool size (0 = auto-detect based on hardware)
constexpr std::size_t kDefaultIoContextPoolSize = 0;

/* -------------------------------------------------------------------------- */
/*                            Strand Pool Defaults                            */
/* -------------------------------------------------------------------------- */

/// Default initial strand pool size
constexpr std::size_t kDefaultStrandPoolInitialSize = 4;

/// Default minimum strand pool size
constexpr std::size_t kDefaultStrandPoolMinSize = 1;

/// Default maximum strand pool size
constexpr std::size_t kDefaultStrandPoolMaxSize = 64;

/// Default strand pool health check interval (5 seconds)
constexpr auto kDefaultStrandHealthCheckInterval = std::chrono::milliseconds(5000);

/// Default strand pool metrics interval (10 seconds)
constexpr auto kDefaultStrandMetricsInterval = std::chrono::milliseconds(10000);

/// Default strand pool load threshold (80%)
constexpr double kDefaultStrandLoadThreshold = 0.8;

/// Default strand pool idle threshold (20%)
constexpr double kDefaultStrandIdleThreshold = 0.2;

/* -------------------------------------------------------------------------- */
/*                           Timer Manager Defaults                           */
/* -------------------------------------------------------------------------- */

/// Default timer bucket count
constexpr std::size_t kDefaultTimerBucketCount = 16;

/// Default timer cleanup interval (60 seconds)
constexpr auto kDefaultTimerCleanupInterval = std::chrono::milliseconds(60000);

/// Default timer statistics interval (30 seconds)
constexpr auto kDefaultTimerStatisticsInterval = std::chrono::milliseconds(30000);

/* -------------------------------------------------------------------------- */
/*                           HTTP Protocol Constants                          */
/* -------------------------------------------------------------------------- */

/// Default HTTP port
constexpr uint16_t kDefaultHttpPort = 80;

/// Default HTTPS port
constexpr uint16_t kDefaultHttpsPort = 443;

/// HTTP/1.1 version number
constexpr unsigned kHttp11Version = 11;

/* -------------------------------------------------------------------------- */
/*                             Temporary Directory                            */
/* -------------------------------------------------------------------------- */

/// Default temporary directory for multipart uploads
constexpr const char* kDefaultTempDirectory = "/tmp";

/* -------------------------------------------------------------------------- */
/*                        HTTP Method Utility Functions                       */
/* -------------------------------------------------------------------------- */

/// Convert HTTP verb to uppercase string (e.g., "GET", "POST", "PUT", "DELETE")
inline const char* verb_to_string(boost::beast::http::verb v) {
  switch (v) {
    case boost::beast::http::verb::get:
      return "GET";
    case boost::beast::http::verb::post:
      return "POST";
    case boost::beast::http::verb::put:
      return "PUT";
    case boost::beast::http::verb::delete_:
      return "DELETE";
    case boost::beast::http::verb::head:
      return "HEAD";
    case boost::beast::http::verb::options:
      return "OPTIONS";
    case boost::beast::http::verb::patch:
      return "PATCH";
    case boost::beast::http::verb::trace:
      return "TRACE";
    case boost::beast::http::verb::connect:
      return "CONNECT";
    default:
      return "UNKNOWN";
  }
}

}  // namespace foxhttp::core
