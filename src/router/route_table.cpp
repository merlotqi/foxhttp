/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <algorithm>
#include <foxhttp/router/route.hpp>
#include <foxhttp/router/route_table.hpp>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace foxhttp {

RouteTable &RouteTable::instance()
{
    static RouteTable inst;
    return inst;
}

static std::string strip_trailing_slash(const std::string &s)
{
    if (s.size() > 1 && s.back() == '/')
    {
        return s.substr(0, s.size() - 1);
    }
    return s;
}

std::string RouteTable::normalize_path(const std::string &path)
{
    if (path.empty())
        return "/";
    // ensure leading slash
    std::string p = path;
    if (p.front() != '/')
        p.insert(p.begin(), '/');
    // remove trailing slash unless root
    p = strip_trailing_slash(p);
    return p;
}

std::size_t RouteTable::count_segments(const std::string &path)
{
    // Count non-empty segments in normalized path ("/a/b" -> 2)
    std::size_t count = 0;
    std::size_t i = 0, n = path.size();
    while (i < n)
    {
        // skip '/'
        while (i < n && path[i] == '/') ++i;
        if (i >= n)
            break;
        // segment start
        ++count;
        while (i < n && path[i] != '/') ++i;
    }
    return count;
}

void RouteTable::add_static_route(const std::string &path, std::shared_ptr<APIHandler> handler)
{
    auto key = normalize_path(path);
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto route = std::make_shared<StaticRoute>(key, std::move(handler));
    static_routes_[key] = route;
    all_routes_.push_back(route);
}

void RouteTable::add_dynamic_route(const std::string &pattern, std::shared_ptr<APIHandler> handler)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);

    try
    {
        auto [regex_pattern, param_names] = RouteBuilder::parse_pattern(pattern);
        auto route = std::make_shared<DynamicRoute>(pattern, std::move(handler), regex_pattern, param_names);

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
                      if (a_specific != b_specific)
                          return a_specific > b_specific;// more specific first
                      // tie-breaker: more segments first (longer path)
                      if (a_total != b_total)
                          return a_total > b_total;
                      return a->pattern() < b->pattern();
                  });
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Failed to parse dynamic route pattern '" + pattern + "': " + e.what());
    }
}

std::shared_ptr<APIHandler> RouteTable::route(const std::string &path, RequestContext &ctx) const
{
    auto key = normalize_path(path);

    // allow concurrent readers
    std::shared_lock<std::shared_mutex> lock(mutex_);

    // 1) exact static match
    auto static_it = static_routes_.find(key);
    if (static_it != static_routes_.end())
    {
        // return shared_ptr to ensure lifetime
        return static_it->second->handler();
    }

    // 2) try dynamic routes (in specificity order)
    for (const auto &route: dynamic_routes_)
    {
        // DynamicRoute::match will set path parameters into ctx when matched
        if (route->match(key, ctx))
        {
            return route->handler();
        }
    }

    // not found
    return nullptr;
}

std::vector<std::shared_ptr<Route>> RouteTable::get_all_routes() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return all_routes_;
}

std::size_t RouteTable::static_route_count() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return static_routes_.size();
}

std::size_t RouteTable::dynamic_route_count() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return dynamic_routes_.size();
}

void RouteTable::clear()
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    static_routes_.clear();
    dynamic_routes_.clear();
    all_routes_.clear();
}

}// namespace foxhttp
