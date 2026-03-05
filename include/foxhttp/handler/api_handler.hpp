/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

namespace foxhttp {

class request_context;
class api_handler {
 public:
  virtual ~api_handler() = default;
  virtual void handle_request(request_context &ctx, http::response<http::string_body> &res) = 0;
};

}  // namespace foxhttp
