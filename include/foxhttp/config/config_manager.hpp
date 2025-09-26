/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <foxhttp/config/configs.hpp>
#include <foxhttp/config/details/config_watcher.hpp>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <shared_mutex>
#include <string>
#include <yaml-cpp/yaml.h>

namespace foxhttp {

class config_manager
{
public:
    using loader_fn = std::function<std::optional<nlohmann::json>(const std::string &path)>;
    using log_fn = std::function<void(const std::string &)>;

    struct snapshot
    {
        json_config json;
        multipart_config multipart;
        form_config form;
        plain_text_config plain;
        strand_pool_config strand_pool;
        timer_manager_config timer_mgr;
    };

    static config_manager &instance();
    bool initialize(const std::string &path, bool enable_watch = true,
                    std::chrono::milliseconds interval = std::chrono::seconds(2));

    void register_loader(const std::string &extension, loader_fn loader);
    bool load_file(const std::string &path);
    bool reload();

    json_config get_json_config() const;
    multipart_config get_multipart_config() const;
    form_config get_form_config() const;
    plain_text_config get_plain_text_config() const;
    const nlohmann::json &document() const;

    std::shared_ptr<const snapshot> get_snapshot() const;
    using diff_callback = std::function<void(const nlohmann::json &new_val, const nlohmann::json &old_val)>;
    void subscribe(const std::string &path, diff_callback cb);

    void start_watching(std::chrono::milliseconds interval = std::chrono::seconds(2));
    void stop_watching();

    void set_logger(log_fn logger);

    using validate_fn = std::function<bool(const nlohmann::json &full_doc, std::string &error)>;
    void set_validator(validate_fn v);

private:
    json_config _json_from_doc() const;
    multipart_config _multipart_from_doc() const;
    form_config _form_from_doc() const;
    plain_text_config _plain_from_doc() const;
    strand_pool_config _strand_pool_from_doc() const;
    timer_manager_config _timer_from_doc() const;

    // Helpers
    static void _deep_merge(nlohmann::json &dst, const nlohmann::json &src);
    static nlohmann::json _json_pointer_at(const nlohmann::json &doc, const std::string &path);
    void _rebuild_snapshot_unlocked();
    void _notify_subscribers_unlocked(const nlohmann::json &new_doc, const nlohmann::json &old_doc);
    void _log(const std::string &msg) const;

private:
    config_manager() = default;

private:
    std::string config_path_;
    nlohmann::json document_;
    nlohmann::json last_good_document_;
    mutable std::shared_mutex mutex_;
    std::mutex loader_mutex_;
    std::map<std::string, loader_fn> loaders_;
    std::unique_ptr<details::config_watcher> watcher_;
    std::shared_ptr<snapshot> snapshot_;

    // subscriptions
    std::mutex sub_mutex_;
    std::map<std::string, std::vector<diff_callback>> subscribers_;

    // hooks
    mutable std::mutex log_mutex_;
    log_fn logger_;
    mutable std::mutex validator_mutex_;
    validate_fn validator_;

    // defaults used for safe merging
    nlohmann::json default_document_ = nlohmann::json{
            {"parsers",
             {{"json",
               {{"maxBodySize", 10 * 1024 * 1024},
                {"maxDepth", 100},
                {"strict", true},
                {"allowComments", false},
                {"allowTrailingCommas", false},
                {"allowDuplicateKeys", true},
                {"validateSchema", false},
                {"charset", "UTF-8"}}},
              {"multipart",
               {{"maxFieldSize", 10 * 1024 * 1024},
                {"maxFileSize", 100 * 1024 * 1024},
                {"maxTotalSize", 500 * 1024 * 1024},
                {"memoryThreshold", 1024 * 1024},
                {"tempDirectory", "/tmp"},
                {"autoCleanup", true},
                {"strict", true},
                {"allowEmptyFields", false}}},
              {"form",
               {{"maxFieldSize", 1024 * 1024},
                {"maxTotalSize", 10 * 1024 * 1024},
                {"maxFields", 1000},
                {"strict", true},
                {"allowEmptyValues", true},
                {"supportArrays", true},
                {"charset", "UTF-8"}}},
              {"plain",
               {{"maxBodySize", 10 * 1024 * 1024},
                {"normalizeLineEndings", true},
                {"trimWhitespace", false},
                {"validateEncoding", true},
                {"charset", "UTF-8"},
                {"allowedContentTypes", nlohmann::json::array()},
                {"strict", false}}}},
             {"core",
              {{"strand_pool",
                {{"initialSize", (int) std::thread::hardware_concurrency()},
                 {"minSize", 1},
                 {"maxSize", 64},
                 {"healthCheckIntervalMs", 5000},
                 {"metricsIntervalMs", 10000},
                 {"autoScaling", true},
                 {"loadThreshold", 0.8},
                 {"idleThreshold", 0.2}}},
               {"timer_manager",
                {{"bucketCount", 16},
                 {"cleanupIntervalMs", 60000},
                 {"statisticsIntervalMs", 30000},
                 {"enableStatistics", true},
                 {"enableCleanup", true}}}}}}
    };
};

}// namespace foxhttp
