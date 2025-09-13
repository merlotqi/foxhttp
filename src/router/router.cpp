/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <foxhttp/router/route_table.hpp>
#include <foxhttp/router/router.hpp>

namespace foxhttp {

void Router::register_static_handler(const std::string &path, std::shared_ptr<APIHandler> handler)
{
    RouteTable::instance().add_static_route(path, std::move(handler));
}

void Router::register_dynamic_handler(const std::string &path, std::shared_ptr<APIHandler> handler)
{
    RouteTable::instance().add_dynamic_route(path, std::move(handler));
}

std::shared_ptr<APIHandler> Router::route(const std::string &path, RequestContext &ctx)
{
    return RouteTable::instance().route(path, ctx);
}

}// namespace foxhttp