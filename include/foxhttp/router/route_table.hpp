/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace foxhttp {

class APIHandler;
class Route;
class RequestContext;
class StaticRoute;
class DynamicRoute;

class RouteTable
{
public:
    static RouteTable &instance();

    void add_static_route(const std::string &path, std::shared_ptr<APIHandler> handler);
    void add_dynamic_route(const std::string &pattern, std::shared_ptr<APIHandler> handler);

    std::shared_ptr<APIHandler> route(const std::string &path, RequestContext &ctx) const;
    std::vector<std::shared_ptr<Route>> get_all_routes() const;

    std::size_t static_route_count() const;
    std::size_t dynamic_route_count() const;
    void clear();

private:
    RouteTable() = default;
    ~RouteTable() = default;
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
};


}// namespace foxhttp