#include <algorithm>
#include <foxhttp/router/route.hpp>
#include <foxhttp/router/route_table.hpp>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace foxhttp::router {

// Static members initialization
std::mutex RouteTable::instances_mutex;
std::unordered_map<std::string, std::shared_ptr<RouteTable>> RouteTable::named_instances;

std::shared_ptr<RouteTable> RouteTable::create_detached() {
  struct MakeSharedEnabler : RouteTable {};
  return std::shared_ptr<RouteTable>(new MakeSharedEnabler());
}

RouteTable &RouteTable::instance() {
  static RouteTable inst;
  return inst;
}

RouteTable &RouteTable::instance(const std::string &name) {
  std::lock_guard<std::mutex> lock(instances_mutex);
  auto it = named_instances.find(name);
  if (it != named_instances.end()) {
    return *it->second;
  }
  // Create new named instance
  auto new_instance = std::shared_ptr<RouteTable>(new RouteTable());
  named_instances[name] = new_instance;
  return *new_instance;
}

bool RouteTable::remove_instance(const std::string &name) {
  std::lock_guard<std::mutex> lock(instances_mutex);
  return named_instances.erase(name) > 0;
}

std::vector<std::string> RouteTable::get_instance_names() {
  std::lock_guard<std::mutex> lock(instances_mutex);
  std::vector<std::string> names;
  names.reserve(named_instances.size());
  for (const auto &[name, _] : named_instances) {
    names.push_back(name);
  }
  return names;
}

static std::string strip_trailing_slash(const std::string &s) {
  if (s.size() > 1 && s.back() == '/') {
    return s.substr(0, s.size() - 1);
  }
  return s;
}

std::string RouteTable::normalize_path(const std::string &path) {
  if (path.empty()) return "/";
  // ensure leading slash
  std::string p = path;
  if (p.front() != '/') p.insert(p.begin(), '/');
  // remove trailing slash unless root
  p = strip_trailing_slash(p);
  return p;
}

std::size_t RouteTable::count_segments(const std::string &path) {
  // Count non-empty segments in normalized path ("/a/b" -> 2)
  std::size_t count = 0;
  std::size_t i = 0, n = path.size();
  while (i < n) {
    // skip '/'
    while (i < n && path[i] == '/') ++i;
    if (i >= n) break;
    // segment start
    ++count;
    while (i < n && path[i] != '/') ++i;
  }
  return count;
}

void RouteTable::add_static_route(const std::string &path, std::shared_ptr<ApiHandler> handler) {
  add_static_route(path, http::verb::unknown, std::move(handler));
}

void RouteTable::add_static_route(const std::string &path, http::verb method, std::shared_ptr<ApiHandler> handler) {
  auto key = normalize_path(path);
  std::unique_lock<std::shared_mutex> lock(mutex_);

  auto route = std::make_shared<StaticRoute>(key, std::move(handler), method);
  static_routes_[key] = route;
  all_routes_.push_back(route);
}

void RouteTable::add_dynamic_route(const std::string &pattern, std::shared_ptr<ApiHandler> handler) {
  add_dynamic_route(pattern, http::verb::unknown, std::move(handler));
}

void RouteTable::add_dynamic_route(const std::string &pattern, http::verb method,
                                    std::shared_ptr<ApiHandler> handler) {
  std::unique_lock<std::shared_mutex> lock(mutex_);

  try {
    auto [regex_pattern, param_names] = RouteBuilder::parse_pattern(pattern);
    auto route = std::make_shared<DynamicRoute>(pattern, std::move(handler), regex_pattern, param_names, method);

    dynamic_routes_.push_back(route);
    all_routes_.push_back(route);

    // resort dynamic routes by "specificity":
    // specificity = number_of_static_segments = total_segments - param_count
    std::sort(dynamic_routes_.begin(), dynamic_routes_.end(),
              [](const std::shared_ptr<DynamicRoute> &a, const std::shared_ptr<DynamicRoute> &b) {
                auto a_total = count_segments(a->pattern());
                auto b_total = count_segments(b->pattern());
                auto a_params = a->param_names().size();
                auto b_params = b->param_names().size();
                auto a_specific = (a_total > a_params) ? (a_total - a_params) : 0;
                auto b_specific = (b_total > b_params) ? (b_total - b_params) : 0;
                if (a_specific != b_specific) return a_specific > b_specific;  // more specific first
                // tie-breaker: more segments first (longer path)
                if (a_total != b_total) return a_total > b_total;
                return a->pattern() < b->pattern();
              });
  } catch (const std::exception &e) {
    throw std::runtime_error("Failed to parse dynamic route pattern '" + pattern + "': " + e.what());
  }
}

std::shared_ptr<ApiHandler> RouteTable::resolve_route(const std::string &path,
                                                       server::RequestContext &ctx) const {
  auto result = resolve_route_with_method(path, http::verb::unknown, ctx);
  return result.first;
}

std::pair<std::shared_ptr<ApiHandler>, std::vector<http::verb>> RouteTable::resolve_route_with_method(
    const std::string &path, http::verb method, server::RequestContext &ctx) const {
  auto key = normalize_path(path);

  // allow concurrent readers
  std::shared_lock<std::shared_mutex> lock(mutex_);

  // Collect allowed methods for 405 response
  std::vector<http::verb> allowed_methods;

  // 1) Check static routes
  auto static_it = static_routes_.find(key);
  if (static_it != static_routes_.end()) {
    auto &route = static_it->second;
    if (route->matches_method(method)) {
      return {route->handler(), {}};
    }
    // Path matches but method doesn't - collect allowed methods
    auto m = route->method();
    if (m != http::verb::unknown) {
      allowed_methods.push_back(m);
    }
  } else {
    // 2) Check dynamic routes for path match
    for (const auto &route : dynamic_routes_) {
      // Try matching path first (without setting params)
      std::smatch matches;
      if (std::regex_match(key, matches, route->regex())) {
        if (route->matches_method(method)) {
          // Method matches, extract params and return handler
          route->extract_path_params(matches, ctx);
          return {route->handler(), {}};
        }
        // Path matches but method doesn't - collect allowed methods
        auto m = route->method();
        if (m != http::verb::unknown) {
          allowed_methods.push_back(m);
        }
        break;  // Only check first matching dynamic route
      }
    }
  }

  // If no handler found but path matched, return 405 info
  if (!allowed_methods.empty()) {
    return {nullptr, allowed_methods};
  }

  // Not found at all
  return {nullptr, {}};
}

std::vector<std::shared_ptr<Route>> RouteTable::get_all_routes() const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  return all_routes_;
}

std::size_t RouteTable::static_route_count() const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  return static_routes_.size();
}

std::size_t RouteTable::dynamic_route_count() const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  return dynamic_routes_.size();
}

void RouteTable::clear() {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  static_routes_.clear();
  dynamic_routes_.clear();
  all_routes_.clear();
}

}  // namespace foxhttp::router
