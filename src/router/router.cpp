#include <foxhttp/router/route_table.hpp>
#include <foxhttp/router/router.hpp>

namespace foxhttp::router {

void Router::register_static_handler(const std::string &path, std::shared_ptr<ApiHandler> handler) {
  RouteTable::instance().add_static_route(path, std::move(handler));
}

void Router::register_dynamic_handler(const std::string &path, std::shared_ptr<ApiHandler> handler) {
  RouteTable::instance().add_dynamic_route(path, std::move(handler));
}

std::shared_ptr<ApiHandler> Router::resolve_route(const std::string &path, server::RequestContext &ctx) {
  return RouteTable::instance().resolve_route(path, ctx);
}

}  // namespace foxhttp::router
