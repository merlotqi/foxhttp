/**
 * foxhttp - Security Headers Middleware
 */

#pragma once

#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/server/request_context.hpp>
#include <boost/beast/http.hpp>
#include <string>

namespace http = boost::beast::http;

namespace foxhttp {
namespace advanced {

class security_headers_middleware : public middleware
{
public:
    security_headers_middleware &hsts(const std::string &value = "max-age=63072000; includeSubDomains; preload")
    {
        hsts_ = value; return *this;
    }
    security_headers_middleware &csp(const std::string &value)
    {
        csp_ = value; return *this;
    }
    security_headers_middleware &frame_options(const std::string &value = "SAMEORIGIN")
    {
        xfo_ = value; return *this;
    }
    security_headers_middleware &content_type_options()
    {
        xcto_ = "nosniff"; return *this;
    }
    security_headers_middleware &referrer_policy(const std::string &value = "no-referrer")
    {
        referrer_ = value; return *this;
    }
    security_headers_middleware &xss_protection(const std::string &value = "0")
    {
        xss_ = value; return *this;
    }

    std::string name() const override { return "security_headers_middleware"; }
    middleware_priority priority() const override { return middleware_priority::high; }

    void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override
    {
        next();
        if (!hsts_.empty()) res.set("Strict-Transport-Security", hsts_);
        if (!csp_.empty()) res.set("Content-Security-Policy", csp_);
        if (!xfo_.empty()) res.set("X-Frame-Options", xfo_);
        if (!xcto_.empty()) res.set("X-Content-Type-Options", xcto_);
        if (!referrer_.empty()) res.set("Referrer-Policy", referrer_);
        if (!xss_.empty()) res.set("X-XSS-Protection", xss_);
    }

private:
    std::string hsts_;
    std::string csp_;
    std::string xfo_ = "SAMEORIGIN";
    std::string xcto_ = "nosniff";
    std::string referrer_ = "no-referrer";
    std::string xss_ = "0";
};

} // namespace advanced
} // namespace foxhttp


