#include "version.h"
#include <boost/json.hpp>

void api_version::handle_request(request_context &ctx, http::response<http::string_body> &res)
{
    if (ctx.method() == http::verb::get)
    {
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");

        boost::json::object obj;
        obj["code"] = 0;
        obj["message"] = "Hello, foxhttp!";
        obj["version"] = "0.0.1";

        res.body() = boost::json::serialize(obj);
        res.prepare_payload();
    }
}