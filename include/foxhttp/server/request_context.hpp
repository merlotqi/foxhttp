#pragma once

#include <any>
#include <boost/beast/http.hpp>
#include <shared_mutex>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace http = boost::beast::http;

namespace foxhttp {

class request_context {
 public:
  explicit request_context(const http::request<http::string_body> &req);
  ~request_context() = default;

  const http::request<http::string_body> &raw_request() const;

  http::verb method() const;
  std::string method_string() const;
  std::string path() const;
  std::string query() const;
  std::string body() const;

  std::string header(const std::string &key, const std::string &default_value = {}) const;
  std::string header(http::field key, const std::string &default_value = {}) const;

  void set_path_parameters(const std::unordered_map<std::string, std::string> &params);
  bool contains_path_parameter(const std::string &key) const;
  std::string path_parameter(const std::string &key) const;
  std::unordered_map<std::string, std::string> path_parameters() const;

  const std::unordered_map<std::string, std::vector<std::string>> &query_parameters() const;

  std::string cookie(const std::string &key) const;
  const std::unordered_map<std::string, std::string> &cookies() const;

  template <typename T>
  void set(const std::string &key, T value);
  template <typename T>
  T get(const std::string &key, const T &default_value = T()) const;
  bool has(const std::string &key) const;

  template <typename T>
  void set_parsed_body(T value);

  template <typename T>
  T parsed_body() const;

 private:
  void parse_query(const std::string &query);
  void parse_cookies();

 private:
  http::request<http::string_body> req_;
  std::unordered_map<std::string, std::string> path_params_;
  std::unordered_map<std::string, std::vector<std::string>> query_params_;
  std::unordered_map<std::string, std::string> cookies_;

  mutable std::shared_mutex items_mutex_;
  std::unordered_map<std::string, std::any> items_;

  mutable std::shared_mutex parsed_body_mutex_;
  std::unordered_map<std::type_index, std::any> parsed_body_cache_;
};

template void request_context::set<std::string>(const std::string &, std::string);
template std::string request_context::get<std::string>(const std::string &, const std::string &) const;
template void request_context::set_parsed_body<std::string>(std::string);
template std::string request_context::parsed_body<std::string>() const;

template <typename T>
void request_context::set(const std::string &key, T value) {
  std::unique_lock lock(items_mutex_);
  items_[key] = std::move(value);
}

template <typename T>
T request_context::get(const std::string &key, const T &default_value) const {
  std::shared_lock lock(items_mutex_);
  auto it = items_.find(key);
  if (it != items_.end()) {
    try {
      return std::any_cast<T>(it->second);
    } catch (...) {
      return default_value;
    }
  }
  return default_value;
}

template <typename T>
void request_context::set_parsed_body(T value) {
  std::unique_lock lock(parsed_body_mutex_);
  parsed_body_cache_[std::type_index(typeid(T))] = std::move(value);
}

template <typename T>
T request_context::parsed_body() const {
  std::shared_lock lock(parsed_body_mutex_);
  auto it = parsed_body_cache_.find(std::type_index(typeid(T)));
  if (it != parsed_body_cache_.end()) {
    try {
      return std::any_cast<T>(it->second);
    } catch (...) {
      throw std::runtime_error("parsed_body type mismatch");
    }
  }
  throw std::runtime_error("parsed_body not found");
}

}  // namespace foxhttp
