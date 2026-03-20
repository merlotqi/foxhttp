#pragma once

#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace foxhttp {

class api_handler;
class route;
class request_context;
class static_route;
class dynamic_route;

class route_table {
 public:
  static route_table &instance();

  void add_static_route(const std::string &path, std::shared_ptr<api_handler> handler);
  void add_dynamic_route(const std::string &pattern, std::shared_ptr<api_handler> handler);

  std::shared_ptr<api_handler> resolve_route(const std::string &path, request_context &ctx) const;
  std::vector<std::shared_ptr<route>> get_all_routes() const;

  std::size_t static_route_count() const;
  std::size_t dynamic_route_count() const;
  void clear();

 private:
  route_table() = default;
  ~route_table() = default;
  route_table(const route_table &) = delete;
  route_table &operator=(const route_table &) = delete;

  // normalize path for consistent matching and storage
  static std::string normalize_path(const std::string &path);
  static std::size_t count_segments(const std::string &path);

 private:
  // mutex for protecting structures:
  // - use shared_mutex so readers (route()) don't block each other
  mutable std::shared_mutex mutex_;

  // static route map: normalized_path -> StaticRoute
  std::unordered_map<std::string, std::shared_ptr<static_route>> static_routes_;

  // dynamic routes (kept in order of specificity)
  std::vector<std::shared_ptr<dynamic_route>> dynamic_routes_;

  // all routes (for introspection)
  std::vector<std::shared_ptr<route>> all_routes_;
};

}  // namespace foxhttp
