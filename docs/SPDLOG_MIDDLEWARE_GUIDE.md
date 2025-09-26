# SPDLog middleware Guide

## Overview

The enhanced `LoggerMiddleware` now uses [spdlog](https://github.com/gabime/spdlog), a fast C++ logging library, providing professional-grade logging capabilities for FoxHTTP applications.

## Key Features

### 🚀 **High Performance**
- **Asynchronous logging**: Non-blocking log operations
- **Multi-threaded**: Thread-safe logging with high concurrency
- **Fast formatting**: Optimized string formatting with fmt library
- **Memory efficient**: Minimal memory allocations

### 📊 **Multiple Log Formats**
- **Simple**: One-line format for basic logging
- **Detailed**: Multi-line format with comprehensive information
- **JSON**: Structured logging for log aggregation systems
- **Apache**: Common Log Format compatible with web server logs

### 🎯 **Flexible Output**
- **Console output**: Colored console logging with different levels
- **File output**: Rotating file logs with size limits
- **Multiple sinks**: Simultaneous console and file logging
- **Configurable levels**: TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL

### 🔧 **Runtime Configuration**
- **Dynamic log level changes**: Adjust logging verbosity at runtime
- **Format switching**: Change log format without restart
- **File management**: Add/remove log files dynamically
- **Console control**: Enable/disable console output

## Usage Examples

### Basic Usage

```cpp
#include <foxhttp/middleware/basic/logger_middleware.hpp>

// Simple console logging
auto logger = std::make_shared<LoggerMiddleware>("MyLogger");
chain.use(logger);
```

### Advanced Configuration

```cpp
// Detailed logging with file output
auto logger = std::make_shared<LoggerMiddleware>(
    "AdvancedLogger",           // Logger name
    LogLevel::DEBUG,           // Log level
    LogFormat::DETAILED,       // Log format
    "app.log",                 // Log file
    true                       // Enable console
);
chain.use(logger);
```

### JSON Structured Logging

```cpp
// JSON format for log aggregation systems (ELK, Splunk, etc.)
auto json_logger = std::make_shared<LoggerMiddleware>(
    "JsonLogger",
    LogLevel::INFO,
    LogFormat::JSON,
    "structured.log"
);
chain.use(json_logger);
```

### Apache Common Log Format

```cpp
// Apache-compatible access logs
auto apache_logger = std::make_shared<LoggerMiddleware>(
    "AccessLogger",
    LogLevel::INFO,
    LogFormat::APACHE,
    "access.log"
);
chain.use(apache_logger);
```

## Log Formats

### 1. Simple Format
```
GET /api/users Mozilla/5.0 [1703123456789-1]
POST /api/login 200 156 2.34ms [1703123456790-2]
```

### 2. Detailed Format
```
Request started - ID: 1703123456789-1, Method: GET, Path: /api/users, IP: 192.168.1.100, User-Agent: Mozilla/5.0
Request completed - ID: 1703123456789-1, Method: GET, Path: /api/users, Status: 200, Size: 1024 bytes, Duration: 2.34ms, Async: false
```

### 3. JSON Format
```json
{"event":"request_start","id":"1703123456789-1","method":"GET","path":"/api/users","ip":"192.168.1.100","user_agent":"Mozilla/5.0","timestamp":"1703123456789"}
{"event":"request_complete","id":"1703123456789-1","method":"GET","path":"/api/users","status":200,"size":1024,"duration_ms":2.34,"async":false,"timestamp":"1703123456791"}
```

### 4. Apache Format
```
192.168.1.100 - - [25/Dec/2023:10:30:56 +0000] "GET /api/users HTTP/1.1" 200 1024 "-" "Mozilla/5.0"
```

## Log Levels

| Level | Description | Use Case |
|-------|-------------|----------|
| `TRACE` | Most verbose | Development debugging |
| `DEBUG` | Debug information | Development and troubleshooting |
| `INFO` | General information | Normal operation monitoring |
| `WARN` | Warning messages | Potential issues |
| `ERROR` | Error conditions | Error tracking |
| `CRITICAL` | Critical errors | System failures |

## Runtime Configuration

### Change Log Level
```cpp
logger->set_log_level(LogLevel::DEBUG);
```

### Change Log Format
```cpp
logger->set_log_format(LogFormat::JSON);
```

### Enable/Disable File Logging
```cpp
logger->set_log_file("new_log.log");  // Enable file logging
logger->set_log_file("");             // Disable file logging
```

### Enable/Disable Console Output
```cpp
logger->set_console_enabled(false);   // Disable console
logger->set_console_enabled(true);    // Enable console
```

## Request Tracking

### Request ID Generation
Each request gets a unique ID for tracking:
- Format: `{timestamp}-{counter}`
- Example: `1703123456789-1`
- Stored in request context for correlation

### Request Lifecycle Logging
- **Start**: Request received and processing begins
- **Complete**: Request processed successfully
- **Error**: Request failed with exception
- **Timeout**: Request exceeded timeout limit
- **Stopped**: Request processing was stopped by middleware

## Performance Considerations

### File Rotation
- **Size-based rotation**: 5MB per file, 3 files max
- **Automatic cleanup**: Old files are automatically removed
- **Zero downtime**: Rotation happens without service interruption

### Memory Usage
- **Minimal allocations**: Efficient memory usage
- **Buffer management**: Optimized buffer handling
- **Thread safety**: Lock-free operations where possible

### Async Operations
- **Non-blocking**: Logging doesn't block request processing
- **Background processing**: File I/O happens in background
- **Error handling**: Graceful handling of logging failures

## Integration with Log Aggregation Systems

### ELK Stack (Elasticsearch, Logstash, Kibana)
```cpp
// JSON format for easy parsing
auto elk_logger = std::make_shared<LoggerMiddleware>(
    "ELKLogger",
    LogLevel::INFO,
    LogFormat::JSON,
    "/var/log/foxhttp/app.json.log"
);
```

### Splunk
```cpp
// Structured logging for Splunk ingestion
auto splunk_logger = std::make_shared<LoggerMiddleware>(
    "SplunkLogger",
    LogLevel::INFO,
    LogFormat::JSON,
    "/opt/splunk/var/log/foxhttp.log"
);
```

### Prometheus + Grafana
```cpp
// Detailed metrics logging
auto metrics_logger = std::make_shared<LoggerMiddleware>(
    "MetricsLogger",
    LogLevel::INFO,
    LogFormat::DETAILED,
    "/var/log/foxhttp/metrics.log"
);
```

## Best Practices

### 1. **Log Level Selection**
```cpp
// Development
logger->set_log_level(LogLevel::DEBUG);

// Production
logger->set_log_level(LogLevel::INFO);

// Critical systems
logger->set_log_level(LogLevel::WARN);
```

### 2. **Format Selection**
```cpp
// Development - human readable
logger->set_log_format(LogFormat::DETAILED);

// Production - machine readable
logger->set_log_format(LogFormat::JSON);

// Web servers - standard format
logger->set_log_format(LogFormat::APACHE);
```

### 3. **File Management**
```cpp
// Separate logs by purpose
auto access_logger = std::make_shared<LoggerMiddleware>("Access", LogLevel::INFO, LogFormat::APACHE, "access.log");
auto error_logger = std::make_shared<LoggerMiddleware>("Error", LogLevel::ERROR, LogFormat::JSON, "error.log");
auto debug_logger = std::make_shared<LoggerMiddleware>("Debug", LogLevel::DEBUG, LogFormat::DETAILED, "debug.log");
```

### 4. **Performance Monitoring**
```cpp
// Monitor logging performance
auto perf_logger = std::make_shared<LoggerMiddleware>(
    "PerformanceLogger",
    LogLevel::WARN,  // Only log warnings and errors
    LogFormat::SIMPLE,
    "performance.log",
    false  // Disable console for performance
);
```

## Error Handling

### Logging Failures
- **Graceful degradation**: Application continues if logging fails
- **Error reporting**: Logging errors are reported to stderr
- **Fallback mechanisms**: Console fallback if file logging fails

### Exception Safety
- **No-throw guarantee**: Logging never throws exceptions
- **Resource cleanup**: Proper cleanup of logging resources
- **Memory safety**: No memory leaks in logging operations

## Migration from Basic Logging

### Before (Basic std::cout)
```cpp
std::cout << "[" << timestamp << "] " << method << " " << path << std::endl;
```

### After (SPDLog)
```cpp
auto logger = std::make_shared<LoggerMiddleware>("MyLogger", LogLevel::INFO, LogFormat::DETAILED);
chain.use(logger);
```

## Dependencies

### Required
- **spdlog**: Fast C++ logging library
- **fmt**: Fast formatting library (included with spdlog)

### Optional
- **Boost.Asio**: For async operations
- **Boost.Beast**: For HTTP handling

## Configuration Examples

### Development Environment
```cpp
auto dev_logger = std::make_shared<LoggerMiddleware>(
    "DevLogger",
    LogLevel::DEBUG,
    LogFormat::DETAILED,
    "dev.log",
    true  // Console enabled for development
);
```

### Production Environment
```cpp
auto prod_logger = std::make_shared<LoggerMiddleware>(
    "ProdLogger",
    LogLevel::INFO,
    LogFormat::JSON,
    "/var/log/foxhttp/app.log",
    false  // Console disabled in production
);
```

### High-Performance Environment
```cpp
auto perf_logger = std::make_shared<LoggerMiddleware>(
    "PerfLogger",
    LogLevel::WARN,  // Only warnings and errors
    LogFormat::SIMPLE,
    "/var/log/foxhttp/performance.log",
    false
);
```

## Conclusion

The SPDLog-based `LoggerMiddleware` provides enterprise-grade logging capabilities with:

- **High performance** and low latency
- **Flexible configuration** for different environments
- **Multiple output formats** for various use cases
- **Runtime configurability** for dynamic adjustments
- **Professional features** like file rotation and structured logging

This makes FoxHTTP suitable for production environments where comprehensive logging and monitoring are essential.
