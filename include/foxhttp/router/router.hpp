/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <memory>

namespace foxhttp {

class RequestContext;
class APIHandler;

class Router
{
public:
    static void register_static_handler(const std::string &path, std::shared_ptr<APIHandler> handler);
    static void register_dynamic_handler(const std::string &path, std::shared_ptr<APIHandler> handler);
    static std::shared_ptr<APIHandler> route(const std::string &path, RequestContext &ctx);
};

}// namespace foxhttp

#define REGISTER_STATIC_HANDLER(Path, HandlerClass)                                           \
    namespace {                                                                               \
    struct HandlerClass##Register                                                             \
    {                                                                                         \
        HandlerClass##Register()                                                              \
        {                                                                                     \
            foxhttp::Router::register_static_handler(Path, std::make_shared<HandlerClass>()); \
        }                                                                                     \
    } HandlerClass##_register;                                                                \
    }

#define REGISTER_DYNAMIC_HANDLER(Path, HandlerClass)                                           \
    namespace {                                                                                \
    struct HandlerClass##Register                                                              \
    {                                                                                          \
        HandlerClass##Register()                                                               \
        {                                                                                      \
            foxhttp::Router::register_dynamic_handler(Path, std::make_shared<HandlerClass>()); \
        }                                                                                      \
    } HandlerClass##_register;                                                                 \
    }
