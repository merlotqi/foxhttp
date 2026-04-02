#include <foxhttp/router/route_table.hpp>
#include <foxhttp/router/router.hpp>

namespace foxhttp::router {

Router::Router(std::shared_ptr<RouteTable> table) : table_(table ? table : RouteTable::create_detached()) {}

void Router::add_static_route(const std::string &path, std::shared_ptr<ApiHandler> handler) {
  table_->add_static_route(path, std::move(handler));
}

void Router::add_static_route(const std::string &path, http::verb method, std::shared_ptr<ApiHandler> handler) {
  table_->add_static_route(path, method, std::move(handler));
}

void Router::add_dynamic_route(const std::string &pattern, std::shared_ptr<ApiHandler> handler) {
  table_->add_dynamic_route(pattern, std::move(handler));
}

void Router::add_dynamic_route(const std::string &pattern, http::verb method, std::shared_ptr<ApiHandler> handler) {
  table_->add_dynamic_route(pattern, method, std::move(handler));
}

std::shared_ptr<ApiHandler> Router::resolver(const std::string &path, server::RequestContext &ctx) const {
  return table_->resolve_route(path, ctx);
}

std::pair<std::shared_ptr<ApiHandler>, std::vector<http::verb>> Router::resolver_with_method(
    const std::string &path, http::verb method, server::RequestContext &ctx) const {
  return table_->resolve_route_with_method(path, method, ctx);
}

std::shared_ptr<RouteTable> Router::table() const { return table_; }

// --- Global singleton API ---

Router &Router::global_router() {
  static Router inst;
  return inst;
}

void Router::register_static_handler(const std::string &path, std::shared_ptr<ApiHandler> handler) {
  global_router().add_static_route(path, std::move(handler));
}

void Router::register_dynamic_handler(const std::string &pattern, std::shared_ptr<ApiHandler> handler) {
  global_router().add_dynamic_route(pattern, std::move(handler));
}

std::shared_ptr<ApiHandler> Router::resolve_route(const std::string &path, server::RequestContext &ctx) {
  return global_router().resolver(path, ctx);
}

}  // namespace foxhttp::router
