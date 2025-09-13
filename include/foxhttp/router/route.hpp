/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <boost/beast/http.hpp>
#include <memory>
#include <regex>
#include <string>
#include <vector>

namespace http = boost::beast::http;

namespace foxhttp {

class RequestContext;
class APIHandler;

enum class RouteType
{
    Static,
    Dynamic
};

class Route
{
public:
    virtual ~Route() = default;
    virtual APIHandler *match(const std::string &path, RequestContext &ctx) const = 0;
    virtual std::string pattern() const = 0;
    virtual RouteType type() const = 0;
};


class StaticRoute : public Route
{
public:
    StaticRoute(std::string pattern, std::shared_ptr<APIHandler> handler);
    APIHandler *match(const std::string &path, RequestContext &ctx) const override;
    std::string pattern() const override;
    RouteType type() const override
    {
        return RouteType::Static;
    }
    std::shared_ptr<APIHandler> handler() const;

private:
    std::string pattern_;
    std::shared_ptr<APIHandler> handler_;
};

class DynamicRoute : public Route
{
public:
    DynamicRoute(std::string pattern, std::shared_ptr<APIHandler> handler, std::regex regex_pattern,
                 std::vector<std::string> param_names);

    APIHandler *match(const std::string &path, RequestContext &ctx) const override;
    std::string pattern() const override;
    RouteType type() const override
    {
        return RouteType::Dynamic;
    }

    const std::regex &regex() const;
    const std::vector<std::string> &param_names() const;
    std::shared_ptr<APIHandler> handler() const;

private:
    void extract_path_params(const std::smatch &matches, RequestContext &ctx) const;

    std::string pattern_;
    std::shared_ptr<APIHandler> handler_;
    std::regex regex_pattern_;
    std::vector<std::string> param_names_;
};

class RouteBuilder
{
public:
    static std::pair<std::regex, std::vector<std::string>> parse_pattern(const std::string &pattern);

private:
    static std::string regex_escape(const std::string &str);
};


}// namespace foxhttp