#pragma once

#include <boost/beast/http.hpp>
#include <foxhttp/handler/api_handler.hpp>
#include <foxhttp/server/request_context.hpp>
#include <memory>
#include <regex>
#include <string>
#include <vector>

namespace http = boost::beast::http;

namespace foxhttp::router {

using ApiHandler = handler::ApiHandler;

enum class RouteType {
  Static,
  Dynamic
};

/// Route base class with optional HTTP method matching
class Route {
 public:
  virtual ~Route() = default;
  virtual ApiHandler *match(const std::string &path, server::RequestContext &ctx) const = 0;
  virtual std::string pattern() const = 0;
  virtual RouteType type() const = 0;

  /// Get the HTTP method this route matches (verb::unknown means any method)
  virtual http::verb method() const { return http::verb::unknown; }

  /// Check if this route matches the given HTTP method
  virtual bool matches_method(http::verb method) const {
    auto m = this->method();
    return m == http::verb::unknown || m == method;
  }
};

class StaticRoute : public Route {
 public:
  StaticRoute(std::string pattern, std::shared_ptr<ApiHandler> handler,
               http::verb method = http::verb::unknown);
  ApiHandler *match(const std::string &path, server::RequestContext &ctx) const override;
  std::string pattern() const override;
  RouteType type() const override { return RouteType::Static; }
  http::verb method() const override;
  std::shared_ptr<ApiHandler> handler() const;
  bool matches_method(http::verb method) const override;

 private:
  std::string pattern_;
  std::shared_ptr<ApiHandler> handler_;
  http::verb method_;
};

class RouteTable;  // forward declaration

class DynamicRoute : public Route {
 public:
  DynamicRoute(std::string pattern, std::shared_ptr<ApiHandler> handler, std::regex regex_pattern,
                std::vector<std::string> param_names,
                http::verb method = http::verb::unknown);

  ApiHandler *match(const std::string &path, server::RequestContext &ctx) const override;
  std::string pattern() const override;
  RouteType type() const override { return RouteType::Dynamic; }
  http::verb method() const override;
  bool matches_method(http::verb method) const override;

  const std::regex &regex() const;
  const std::vector<std::string> &param_names() const;
  std::shared_ptr<ApiHandler> handler() const;

  // Allow RouteTable to call extract_path_params for method-aware resolution
  friend RouteTable;

 private:
  void extract_path_params(const std::smatch &matches, server::RequestContext &ctx) const;

  std::string pattern_;
  std::shared_ptr<ApiHandler> handler_;
  std::regex regex_pattern_;
  std::vector<std::string> param_names_;
  http::verb method_;
};

class RouteBuilder {
 public:
  static std::pair<std::regex, std::vector<std::string>> parse_pattern(const std::string &pattern);

 private:
  static std::string regex_escape(const std::string &str);
};

}  // namespace foxhttp::router
