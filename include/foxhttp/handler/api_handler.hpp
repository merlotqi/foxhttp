#pragma once

#include <boost/beast/http.hpp>
#include <foxhttp/server/request_context.hpp>

namespace http = boost::beast::http;

namespace foxhttp::handler {

class ApiHandler {
 public:
  virtual ~ApiHandler() = default;
  virtual void handle_request(server::RequestContext &ctx, http::response<http::string_body> &res) = 0;
};

}  // namespace foxhttp::handler
