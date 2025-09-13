/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>

namespace foxhttp {

// 配置值类型
using config_value = std::variant<std::string, int, int64_t, uint32_t, uint64_t, double, bool, std::vector<std::string>,
                                  std::vector<int>, std::vector<double>>;

// 服务器配置
struct server_config
{
    std::string host = "0.0.0.0";
    uint16_t port = 8080;
    int thread_pool_size = std::thread::hardware_concurrency();
    std::chrono::seconds request_timeout{30};
    std::size_t max_request_size = 10 * 1024 * 1024;// 10MB
    bool enable_compression = true;
    std::string compression_level = "default";
    std::size_t max_connections = 10000;
    std::chrono::seconds keep_alive_timeout{15};
    bool reuse_address = true;
    bool reuse_port = false;
    int backlog = 1024;
};

// SSL/TLS 配置
struct ssl_config
{
    bool enabled = false;
    std::string certificate_file;
    std::string private_key_file;
    std::string certificate_chain_file;
    std::string dh_file;
    std::string password;
    std::string cipher_list = "HIGH:!aNULL:!MD5";
    bool require_verification = false;
    std::string ca_certificate_file;
    std::string tls_version = "TLSv1.2";
};

// 日志配置
struct logging_config
{
    std::string level = "info";
    std::string file_path;
    std::size_t max_file_size = 10 * 1024 * 1024;// 10MB
    std::size_t max_files = 10;
    bool console_output = true;
    bool async_logging = true;
    std::string format = "[%Y-%m-%d %H:%M:%S] [%l] %v";
    std::chrono::seconds flush_interval{3};
};

// 路由配置
struct routing_config
{
    bool case_sensitive = false;
    bool strict_routing = true;
    std::string default_content_type = "application/json";
    bool auto_options = true;
    std::size_t max_route_cache_size = 1000;
};

// 中间件配置
struct middleware_config
{
    bool enable_cors = true;
    bool enable_compression = true;
    bool enable_logging = true;
    bool enable_body_parsing = true;
    bool enable_static_files = true;

    // CORS 配置
    std::string cors_allow_origin = "*";
    std::vector<std::string> cors_allow_methods = {"GET", "POST", "PUT", "DELETE", "OPTIONS", "PATCH"};
    std::vector<std::string> cors_allow_headers = {"Content-Type", "Authorization", "X-Requested-With"};
    bool cors_allow_credentials = false;
    std::chrono::seconds cors_max_age{86400};// 24 hours

    // 速率限制配置
    bool enable_rate_limiting = false;
    std::size_t rate_limit_max_requests = 100;
    std::chrono::seconds rate_limit_window{60};

    // 静态文件配置
    std::string static_files_root = "./public";
    std::vector<std::string> static_files_index = {"index.html", "index.htm"};
    bool static_files_cache = true;
    std::chrono::seconds static_files_max_age{3600};// 1 hour
};

// 数据库连接池配置
struct database_config
{
    bool enabled = false;
    std::string type;// "mysql", "postgresql", "sqlite"
    std::string host;
    uint16_t port;
    std::string database;
    std::string username;
    std::string password;
    std::size_t pool_size = 10;
    std::chrono::seconds connection_timeout{30};
    std::chrono::seconds idle_timeout{300};
    std::size_t max_lifetime_connections = 1000;
    bool ssl_enabled = false;
};

// 缓存配置
struct cache_config
{
    bool enabled = false;
    std::string type;// "memory", "redis", "memcached"
    std::string host;
    uint16_t port;
    std::size_t max_size = 10000;
    std::chrono::seconds default_ttl{300};// 5 minutes
    bool cluster_mode = false;
    std::string password;
};

// API 配置
struct api_config
{
    bool enable_swagger = false;
    std::string swagger_path = "/swagger";
    std::string api_prefix = "/api";
    bool enable_versioning = false;
    std::string default_version = "v1";
    bool enable_health_check = true;
    std::string health_check_path = "/health";
};

// 监控配置
struct monitoring_config
{
    bool enable_metrics = false;
    std::string metrics_path = "/metrics";
    std::chrono::seconds metrics_interval{60};
    bool enable_profiling = false;
    std::string profiling_path = "/debug/pprof";
    bool enable_status = true;
    std::string status_path = "/status";
};

// 安全配置
struct security_config
{
    bool enable_https_redirect = false;
    std::size_t max_login_attempts = 5;
    std::chrono::minutes login_lockout_duration{15};
    std::string jwt_secret;
    std::chrono::hours jwt_expiration{24};
    bool enable_csrf = false;
    std::string csrf_token_header = "X-CSRF-Token";
    bool enable_xss_protection = true;
    bool enable_hsts = true;
    std::chrono::seconds hsts_max_age{31536000};// 1 year
};

// 主配置结构
struct config
{
    server_config server;
    ssl_config ssl;
    logging_config logging;
    routing_config routing;
    middleware_config middleware;
    database_config database;
    cache_config cache;
    api_config api;
    monitoring_config monitoring;
    security_config security;

    // 自定义配置项
    std::unordered_map<std::string, config_value> custom;

    // 环境名称
    std::string environment = "development";

    // 配置文件路径
    std::string config_file_path;

    // 配置版本
    std::string version = "1.0.0";
};

// 配置管理器类
class config_manager
{
public:
    // 单例访问
    static config_manager &instance();

    // 加载配置
    bool load(const std::string &config_path = "");
    bool load_from_string(const std::string &config_content, const std::string &format = "json");

    // 重新加载配置
    bool reload();

    // 获取配置
    const config &get() const;
    config &get_mutable();

    // 获取配置值
    std::optional<config_value> get_value(const std::string &key) const;
    template<typename T>
    std::optional<T> get_value_as(const std::string &key) const;

    // 设置配置值
    void set_value(const std::string &key, const config_value &value);

    // 检查配置是否已加载
    bool is_loaded() const;

    // 获取配置文件的最后修改时间
    std::chrono::system_clock::time_point last_modified() const;

    // 注册配置变更回调
    using config_change_callback = std::function<void(const config &new_config, const config &old_config)>;
    void register_change_callback(const std::string &name, config_change_callback callback);
    void unregister_change_callback(const std::string &name);

    // 验证配置
    bool validate() const;
    std::vector<std::string> get_validation_errors() const;

    // 保存配置（如果支持）
    bool save(const std::string &path = "") const;

    // 环境相关方法
    std::string get_environment() const;
    bool is_development() const;
    bool is_production() const;
    bool is_testing() const;

    // 监听配置文件变化（用于热重载）
    void start_watching(std::chrono::milliseconds interval = std::chrono::seconds(5));
    void stop_watching();

private:
    config_manager() = default;
    ~config_manager() = default;

    config_manager(const config_manager &) = delete;
    config_manager &operator=(const config_manager &) = delete;

    config current_config_;
    bool loaded_ = false;
    std::chrono::system_clock::time_point last_modified_;
    std::unordered_map<std::string, config_change_callback> change_callbacks_;
    mutable std::mutex mutex_;
};

// 便捷访问函数
const config &get_config();
const server_config &get_server_config();
const ssl_config &get_ssl_config();
const logging_config &get_logging_config();

// 环境检测宏
#define CONFIG_DEVELOPMENT() (foxhttp::config_manager::instance().is_development())
#define CONFIG_PRODUCTION() (foxhttp::config_manager::instance().is_production())
#define CONFIG_TESTING() (foxhttp::config_manager::instance().is_testing())

// 配置访问宏
#define GET_CONFIG_VALUE(key) foxhttp::config_manager::instance().get_value(key)
#define GET_CONFIG_VALUE_AS(type, key) foxhttp::config_manager::instance().get_value_as<type>(key)

}// namespace foxhttp