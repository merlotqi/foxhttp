#include <foxhttp/server/request_context.hpp>
#include <sstream>

namespace foxhttp::server {

static std::string url_decode(const std::string &str) {
  std::string ret;
  ret.reserve(str.size());
  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '%') {
      if (i + 2 < str.size()) {
        try {
          int value = std::stoi(str.substr(i + 1, 2), nullptr, 16);
          ret += static_cast<char>(value);
          i += 2;
        } catch (...) {
          ret += '%';
        }
      } else {
        ret += '%';
      }
    } else if (str[i] == '+') {
      ret += ' ';
    } else {
      ret += str[i];
    }
  }
  return ret;
}

RequestContext::RequestContext(const http::request<http::string_body> &req) : req_(req) {
  auto target = req_.target();
  auto pos = target.find('?');
  if (pos != std::string::npos) parse_query(target.substr(pos + 1));

  parse_cookies();
}

const http::request<http::string_body> &RequestContext::raw_request() const { return req_; }

http::verb RequestContext::method() const { return req_.method(); }

std::string RequestContext::method_string() const { return req_.method_string(); }

std::string RequestContext::path() const {
  auto target = req_.target();
  auto pos = target.find('?');
  if (pos != boost::beast::string_view::npos) return std::string(target.substr(0, pos));
  return std::string(target);
}

std::string RequestContext::query() const {
  auto target = req_.target();
  auto pos = target.find('?');
  return (pos != std::string::npos) ? target.substr(pos + 1) : "";
}

std::string RequestContext::body() const { return req_.body(); }

std::string RequestContext::header(const std::string &key, const std::string &default_value) const {
  auto it = req_.find(key);
  return (it != req_.end()) ? std::string(it->value()) : default_value;
}

std::string RequestContext::header(http::field key, const std::string &default_value) const {
  auto it = req_.find(key);
  return (it != req_.end()) ? std::string(it->value()) : default_value;
}

// ------------------- path / query -------------------
void RequestContext::set_path_parameters(const std::unordered_map<std::string, std::string> &params) {
  path_params_ = params;
}

bool RequestContext::contains_path_parameter(const std::string &key) const {
  return path_params_.find(key) != path_params_.end();
}

std::string RequestContext::path_parameter(const std::string &key) const {
  auto it = path_params_.find(key);
  return it != path_params_.end() ? it->second : "";
}

std::unordered_map<std::string, std::string> RequestContext::path_parameters() const { return path_params_; }

const std::unordered_map<std::string, std::vector<std::string>> &RequestContext::query_parameters() const {
  return query_params_;
}

void RequestContext::parse_query(const std::string &query) {
  query_params_.clear();
  std::istringstream iss(query);
  std::string pair;

  while (std::getline(iss, pair, '&')) {
    if (pair.empty()) continue;

    auto pos = pair.find('=');
    std::string key, value;
    if (pos != std::string::npos) {
      key = url_decode(pair.substr(0, pos));
      value = url_decode(pair.substr(pos + 1));
    } else {
      key = url_decode(pair);
      value = "";
    }

    std::istringstream vs(value);
    std::string v;
    while (std::getline(vs, v, ','))
      if (!v.empty()) query_params_[key].push_back(v);
  }
}

// ------------------- cookies -------------------
void RequestContext::parse_cookies() {
  cookies_.clear();
  auto it = req_.find(http::field::cookie);
  if (it == req_.end()) return;

  std::istringstream iss(it->value());
  std::string pair;
  while (std::getline(iss, pair, ';')) {
    auto pos = pair.find('=');
    if (pos != std::string::npos) {
      std::string key = pair.substr(0, pos);
      std::string value = pair.substr(pos + 1);

      key.erase(0, key.find_first_not_of(" \t"));
      key.erase(key.find_last_not_of(" \t") + 1);
      value.erase(0, value.find_first_not_of(" \t"));
      value.erase(value.find_last_not_of(" \t") + 1);

      cookies_[key] = value;
    }
  }
}

std::string RequestContext::cookie(const std::string &key) const {
  auto it = cookies_.find(key);
  return it != cookies_.end() ? it->second : "";
}

const std::unordered_map<std::string, std::string> &RequestContext::cookies() const { return cookies_; }

bool RequestContext::has(const std::string &key) const {
  std::shared_lock lock(items_mutex_);
  return items_.find(key) != items_.end();
}

template void RequestContext::set<std::string>(const std::string &, std::string);
template std::string RequestContext::get<std::string>(const std::string &, const std::string &) const;
template void RequestContext::set_parsed_body<std::string>(std::string);
template std::string RequestContext::parsed_body<std::string>() const;

}  // namespace foxhttp::server
