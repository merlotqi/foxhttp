#pragma once

#include <foxhttp/handler/api_handler.hpp>
#include <foxhttp/server/request_context.hpp>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace foxhttp::router {

using ApiHandler = handler::ApiHandler;

class Route;
class StaticRoute;
class DynamicRoute;

class RouteTable {
 public:
  ~RouteTable() = default;

  /// Get the default global route table (singleton)
  static RouteTable &instance();

  /// Get or create a named route table (for multi-server scenarios)
  /// @param name Unique name for the route table
  /// @return Reference to the named route table
  static RouteTable &instance(const std::string &name);

  /// Remove a named route table
  /// @param name Name of the route table to remove
  /// @return true if removed, false if not found
  static bool remove_instance(const std::string &name);

  /// Get all named route table names
  static std::vector<std::string> get_instance_names();

  void add_static_route(const std::string &path, std::shared_ptr<ApiHandler> handler);
  void add_dynamic_route(const std::string &pattern, std::shared_ptr<ApiHandler> handler);

  std::shared_ptr<ApiHandler> resolve_route(const std::string &path, server::RequestContext &ctx) const;
  std::vector<std::shared_ptr<Route>> get_all_routes() const;

  std::size_t static_route_count() const;
  std::size_t dynamic_route_count() const;
  void clear();

 private:
  RouteTable() = default;
  RouteTable(const RouteTable &) = delete;
  RouteTable &operator=(const RouteTable &) = delete;

  // normalize path for consistent matching and storage
  static std::string normalize_path(const std::string &path);
  static std::size_t count_segments(const std::string &path);

 private:
  // mutex for protecting structures:
  // - use shared_mutex so readers (route()) don't block each other
  mutable std::shared_mutex mutex_;

  // static route map: normalized_path -> StaticRoute
  std::unordered_map<std::string, std::shared_ptr<StaticRoute>> static_routes_;

  // dynamic routes (kept in order of specificity)
  std::vector<std::shared_ptr<DynamicRoute>> dynamic_routes_;

  // all routes (for introspection)
  std::vector<std::shared_ptr<Route>> all_routes_;

  // static mutex for protecting named instances map
  static std::mutex instances_mutex;
  // named route tables for multi-server scenarios
  static std::unordered_map<std::string, std::shared_ptr<RouteTable>> named_instances;
};

}  // namespace foxhttp::router
