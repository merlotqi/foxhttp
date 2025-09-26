/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
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

class request_context;
class api_handler;

enum class route_type
{
    static_,
    dynamic
};

class route
{
public:
    virtual ~route() = default;
    virtual api_handler *match(const std::string &path, request_context &ctx) const = 0;
    virtual std::string pattern() const = 0;
    virtual route_type type() const = 0;
};


class static_route : public route
{
public:
    static_route(std::string pattern, std::shared_ptr<api_handler> handler);
    api_handler *match(const std::string &path, request_context &ctx) const override;
    std::string pattern() const override;
    route_type type() const override
    {
        return route_type::static_;
    }
    std::shared_ptr<api_handler> handler() const;

private:
    std::string pattern_;
    std::shared_ptr<api_handler> handler_;
};

class dynamic_route : public route
{
public:
    dynamic_route(std::string pattern, std::shared_ptr<api_handler> handler, std::regex regex_pattern,
                  std::vector<std::string> param_names);

    api_handler *match(const std::string &path, request_context &ctx) const override;
    std::string pattern() const override;
    route_type type() const override
    {
        return route_type::dynamic;
    }

    const std::regex &regex() const;
    const std::vector<std::string> &param_names() const;
    std::shared_ptr<api_handler> handler() const;

private:
    void _extract_path_params(const std::smatch &matches, request_context &ctx) const;

    std::string pattern_;
    std::shared_ptr<api_handler> handler_;
    std::regex regex_pattern_;
    std::vector<std::string> param_names_;
};

class route_builder
{
public:
    static std::pair<std::regex, std::vector<std::string>> parse_pattern(const std::string &pattern);

private:
    static std::string _regex_escape(const std::string &str);
};


}// namespace foxhttp