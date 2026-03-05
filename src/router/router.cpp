/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <foxhttp/router/router.hpp>
// include route table
#include <foxhttp/router/route_table.hpp>

namespace foxhttp {

void router::register_static_handler(const std::string &path, std::shared_ptr<api_handler> handler) {
  route_table::instance().add_static_route(path, std::move(handler));
}

void router::register_dynamic_handler(const std::string &path, std::shared_ptr<api_handler> handler) {
  route_table::instance().add_dynamic_route(path, std::move(handler));
}

std::shared_ptr<api_handler> router::resolve_route(const std::string &path, request_context &ctx) {
  return route_table::instance().resolve_route(path, ctx);
}

}  // namespace foxhttp