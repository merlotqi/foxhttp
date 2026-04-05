#include <yaml-cpp/yaml.h>

#include <foxhttp/config/config_manager.hpp>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>

namespace foxhttp::config {

static std::optional<nlohmann::json> load_json_static(const std::string &path) {
  std::ifstream ifs(path);
  if (!ifs.is_open()) return std::nullopt;
  try {
    return nlohmann::json::parse(ifs);
  } catch (...) {
    return std::nullopt;
  }
}

static nlohmann::json yaml_to_json(const YAML::Node &node) {
  if (node.IsNull()) return nullptr;
  if (node.IsScalar()) return node.as<std::string>();
  if (node.IsSequence()) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto &it : node) arr.push_back(yaml_to_json(it));
    return arr;
  }
  if (node.IsMap()) {
    nlohmann::json obj = nlohmann::json::object();
    for (const auto &it : node) {
      obj[it.first.as<std::string>()] = yaml_to_json(it.second);
    }
    return obj;
  }
  return nullptr;
}

static std::optional<nlohmann::json> load_yaml_static(const std::string &path) {
  try {
    YAML::Node y = YAML::LoadFile(path);
    return yaml_to_json(y);
  } catch (...) {
    return std::nullopt;
  }
}

ConfigManager &ConfigManager::instance() {
  static ConfigManager inst;
  return inst;
}

// Initialize with a file path and optionally start internal watcher
bool ConfigManager::initialize(const std::string &path, bool enable_watch, std::chrono::milliseconds interval) {
  config_path_ = path;
  bool ok = load_file(config_path_);
  if (enable_watch) {
    start_watching(interval);
  }
  return ok;
}

void ConfigManager::register_loader(const std::string &extension, loader_fn loader) {
  std::lock_guard<std::mutex> lk(loader_mutex_);
  loaders_[extension] = std::move(loader);
}

bool ConfigManager::load_file(const std::string &path) {
  std::filesystem::path p(path);
  auto ext = p.extension().string();
  loader_fn loader;
  {
    std::lock_guard<std::mutex> lk(loader_mutex_);
    auto it = loaders_.find(ext);
    if (it != loaders_.end()) loader = it->second;
  }
  if (!loader) {
    // Built-ins
    if (ext == ".json")
      loader = [](const std::string &p) { return load_json_static(p); };
    else if (ext == ".yml" || ext == ".yaml")
      loader = [](const std::string &p) { return load_yaml_static(p); };
  }
  if (!loader) return false;

  std::optional<nlohmann::json> doc = loader(path);
  if (!doc) return false;

  // Prepare merged document outside the lock
  nlohmann::json merged = default_document_;
  deep_merge(merged, *doc);

  // optional validation hook (outside main lock)
  std::string err;
  {
    std::lock_guard<std::mutex> vk(validator_mutex_);
    if (validator_ && !validator_(merged, err)) {
      log(std::string("config validation failed: ") + err);
      return false;  // keep old_doc and snapshot
    }
  }

  // Atomic update: acquire lock, swap document and rebuild snapshot
  nlohmann::json old_doc;
  {
    std::unique_lock<std::shared_mutex> lk(mutex_);
    old_doc = std::move(document_);
    document_ = std::move(merged);
    last_good_document_ = document_;
    rebuild_snapshot_unlocked();
  }
  // Notify subscribers outside the lock to prevent deadlocks
  notify_subscribers_unlocked(document_, old_doc);
  return true;
}

// Reload using remembered path (safe no-op if unset)
bool ConfigManager::reload() {
  if (config_path_.empty()) return false;
  return load_file(config_path_);
}

// Accessors for known sections
JsonConfig ConfigManager::get_json_config() const {
  std::shared_lock<std::shared_mutex> lk(mutex_);
  return json_from_doc();
}
MultipartConfig ConfigManager::get_multipart_config() const {
  std::shared_lock<std::shared_mutex> lk(mutex_);
  return multipart_from_doc();
}
FormConfig ConfigManager::get_form_config() const {
  std::shared_lock<std::shared_mutex> lk(mutex_);
  return form_from_doc();
}
PlainTextConfig ConfigManager::get_plain_text_config() const {
  std::shared_lock<std::shared_mutex> lk(mutex_);
  return plain_from_doc();
}

const nlohmann::json &ConfigManager::document() const { return document_; }

std::shared_ptr<const ConfigManager::ConfigSnapshot> ConfigManager::get_snapshot() const {
  std::shared_lock<std::shared_mutex> lk(mutex_);
  return snapshot_;
}

void ConfigManager::subscribe(const std::string &path, diff_callback cb) {
  std::lock_guard<std::mutex> lk(sub_mutex_);
  subscribers_[path].push_back(std::move(cb));
}

void ConfigManager::set_logger(log_fn logger) {
  std::lock_guard<std::mutex> lk(log_mutex_);
  logger_ = std::move(logger);
}

void ConfigManager::set_validator(validate_fn v) {
  std::lock_guard<std::mutex> lk(validator_mutex_);
  validator_ = std::move(v);
}

void ConfigManager::start_watching(std::chrono::milliseconds interval) {
  if (config_path_.empty()) return;
  if (!watcher_) {
    watcher_ = std::make_unique<detail::ConfigWatcher>(config_path_, interval);
    watcher_->on_reload.connect([this] { /* document_ already merged in load_file */ });
  }
  watcher_->start();
}

void ConfigManager::stop_watching() {
  if (watcher_) watcher_->stop();
}

JsonConfig ConfigManager::json_from_doc() const {
  JsonConfig cfg;
  auto j = document_.value("parsers", nlohmann::json{}).value("json", nlohmann::json{});
  if (!j.is_object()) return cfg;
  cfg.max_size = j.value("maxBodySize", cfg.max_size);
  cfg.max_depth = j.value("maxDepth", cfg.max_depth);
  cfg.strict_mode = j.value("strict", cfg.strict_mode);
  cfg.allow_comments = j.value("allowComments", cfg.allow_comments);
  cfg.allow_trailing_commas = j.value("allowTrailingCommas", cfg.allow_trailing_commas);
  cfg.allow_duplicate_keys = j.value("allowDuplicateKeys", cfg.allow_duplicate_keys);
  cfg.validate_schema = j.value("validateSchema", cfg.validate_schema);
  if (j.contains("schema") && j["schema"].is_object()) cfg.schema = j["schema"];
  cfg.charset = j.value("charset", cfg.charset);
  return cfg;
}

MultipartConfig ConfigManager::multipart_from_doc() const {
  MultipartConfig cfg;
  auto m = document_.value("parsers", nlohmann::json{}).value("multipart", nlohmann::json{});
  if (!m.is_object()) return cfg;
  cfg.max_field_size = m.value("maxFieldSize", cfg.max_field_size);
  cfg.max_file_size = m.value("maxFileSize", cfg.max_file_size);
  cfg.max_total_size = m.value("maxTotalSize", cfg.max_total_size);
  cfg.memory_threshold = m.value("memoryThreshold", cfg.memory_threshold);
  cfg.temp_directory = m.value("tempDirectory", cfg.temp_directory);
  cfg.auto_cleanup = m.value("autoCleanup", cfg.auto_cleanup);
  cfg.strict_mode = m.value("strict", cfg.strict_mode);
  cfg.allow_empty_fields = m.value("allowEmptyFields", cfg.allow_empty_fields);
  if (m.contains("allowedExtensions")) cfg.allowed_extensions = m["allowedExtensions"].get<std::vector<std::string>>();
  if (m.contains("allowedContentTypes"))
    cfg.allowed_content_types = m["allowedContentTypes"].get<std::vector<std::string>>();
  return cfg;
}

FormConfig ConfigManager::form_from_doc() const {
  FormConfig cfg;
  auto f = document_.value("parsers", nlohmann::json{}).value("form", nlohmann::json{});
  if (!f.is_object()) return cfg;
  cfg.max_field_size = f.value("maxFieldSize", cfg.max_field_size);
  cfg.max_total_size = f.value("maxTotalSize", cfg.max_total_size);
  cfg.max_fields = f.value("maxFields", cfg.max_fields);
  cfg.strict_mode = f.value("strict", cfg.strict_mode);
  cfg.allow_empty_values = f.value("allowEmptyValues", cfg.allow_empty_values);
  cfg.support_arrays = f.value("supportArrays", cfg.support_arrays);
  cfg.charset = f.value("charset", cfg.charset);
  return cfg;
}

PlainTextConfig ConfigManager::plain_from_doc() const {
  PlainTextConfig cfg;
  auto p = document_.value("parsers", nlohmann::json{}).value("plain", nlohmann::json{});
  if (!p.is_object()) return cfg;
  cfg.max_size = p.value("maxBodySize", cfg.max_size);
  cfg.normalize_line_endings = p.value("normalizeLineEndings", cfg.normalize_line_endings);
  cfg.trim_whitespace = p.value("trimWhitespace", cfg.trim_whitespace);
  cfg.validate_encoding = p.value("validateEncoding", cfg.validate_encoding);
  cfg.charset = p.value("charset", cfg.charset);
  if (p.contains("allowedContentTypes"))
    cfg.allowed_content_types = p["allowedContentTypes"].get<std::vector<std::string>>();
  cfg.strict_mode = p.value("strict", cfg.strict_mode);
  return cfg;
}

StrandPoolConfig ConfigManager::strand_pool_from_doc() const {
  StrandPoolConfig cfg;
  auto s = document_.value("core", nlohmann::json{}).value("StrandPool", nlohmann::json{});
  if (!s.is_object()) return cfg;
  cfg.initial_size = s.value("initialSize", (int)cfg.initial_size);
  cfg.min_size = s.value("minSize", (int)cfg.min_size);
  cfg.max_size = s.value("maxSize", (int)cfg.max_size);
  cfg.health_check_interval =
      std::chrono::milliseconds{s.value("healthCheckIntervalMs", (int)cfg.health_check_interval.count())};
  cfg.metrics_report_interval =
      std::chrono::milliseconds{s.value("metricsIntervalMs", (int)cfg.metrics_report_interval.count())};
  cfg.enable_auto_scaling = s.value("autoScaling", cfg.enable_auto_scaling);
  cfg.load_threshold = s.value("loadThreshold", cfg.load_threshold);
  cfg.idle_threshold = s.value("idleThreshold", cfg.idle_threshold);
  return cfg;
}

TimerManagerConfig ConfigManager::timer_from_doc() const {
  TimerManagerConfig cfg;
  auto t = document_.value("core", nlohmann::json{}).value("TimerManager", nlohmann::json{});
  if (!t.is_object()) return cfg;
  cfg.bucket_count = t.value("bucketCount", (int)cfg.bucket_count);
  cfg.cleanup_interval = std::chrono::milliseconds{t.value("cleanupIntervalMs", (int)cfg.cleanup_interval.count())};
  cfg.statistics_report_interval =
      std::chrono::milliseconds{t.value("statisticsIntervalMs", (int)cfg.statistics_report_interval.count())};
  cfg.enable_statistics = t.value("enableStatistics", cfg.enable_statistics);
  cfg.enable_cleanup = t.value("enableCleanup", cfg.enable_cleanup);
  return cfg;
}

void ConfigManager::rebuild_snapshot_unlocked() {
  auto snap = std::make_shared<ConfigSnapshot>();
  // Increment version number for change detection
  static uint64_t version_counter = 0;
  snap->version = ++version_counter;
  snap->json = json_from_doc();
  snap->multipart = multipart_from_doc();
  snap->form = form_from_doc();
  snap->plain = plain_from_doc();
  snap->strand_pool = strand_pool_from_doc();
  snap->timer_mgr = timer_from_doc();
  snapshot_ = std::move(snap);
}

static const nlohmann::json *walk_dot_path(const nlohmann::json &node, const std::string &path) {
  if (path.empty()) return &node;
  const nlohmann::json *current = &node;
  size_t start = 0;
  while (start <= path.size()) {
    auto dot = path.find('.', start);
    auto key = path.substr(start, dot == std::string::npos ? path.size() - start : dot - start);
    if (!current->is_object()) return nullptr;
    auto it = current->find(key);
    if (it == current->end()) return nullptr;
    current = &(*it);
    if (dot == std::string::npos) break;
    start = dot + 1;
  }
  return current;
}

void ConfigManager::deep_merge(nlohmann::json &dst, const nlohmann::json &src) {
  if (!src.is_object() || !dst.is_object()) {
    dst = src;
    return;
  }

  for (auto it = src.begin(); it != src.end(); ++it) {
    const auto &key = it.key();
    if (dst.find(key) != dst.end() && dst[key].is_object() && it.value().is_object()) {
      deep_merge(dst[key], it.value());
    } else {
      dst[key] = it.value();
    }
  }
}

nlohmann::json ConfigManager::json_pointer_at(const nlohmann::json &doc, const std::string &path) {
  auto ptr = walk_dot_path(doc, path);
  if (!ptr) return nlohmann::json();
  return *ptr;
}

void ConfigManager::notify_subscribers_unlocked(const nlohmann::json &new_doc, const nlohmann::json &old_doc) {
  std::map<std::string, std::vector<diff_callback>> subs_copy;
  {
    std::lock_guard<std::mutex> lk(sub_mutex_);
    subs_copy = subscribers_;
  }
  for (auto &[path, cbs] : subs_copy) {
    auto old_v = json_pointer_at(old_doc, path);
    auto new_v = json_pointer_at(new_doc, path);
    if (old_v == new_v) continue;
    for (auto &cb : cbs) {
      try {
        cb(new_v, old_v);
      } catch (...) { /* swallow */
      }
    }
  }
}

void ConfigManager::log(const std::string &msg) const {
  std::lock_guard<std::mutex> lk(log_mutex_);
  if (logger_) logger_(msg);
}

}  // namespace foxhttp::config
