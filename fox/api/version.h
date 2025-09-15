#pragma once

#include <foxhttp/foxhttp.hpp>

using namespace  foxhttp;

class api_version : public api_handler
{
public:
    void handle_request(request_context &ctx, http::response<http::string_body> &res) override;
};
REGISTER_STATIC_HANDLER("/version", api_version);