/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <memory>
#include <string>

namespace foxhttp {

class request_context;
class api_handler;

class router
{
public:
    static void register_static_handler(const std::string &path, std::shared_ptr<api_handler> handler);
    static void register_dynamic_handler(const std::string &path, std::shared_ptr<api_handler> handler);
    static std::shared_ptr<api_handler> resolve_route(const std::string &path, request_context &ctx);
};

}// namespace foxhttp

#define REGISTER_STATIC_HANDLER(Path, HandlerClass)                                           \
    namespace {                                                                               \
    struct HandlerClass##Register                                                             \
    {                                                                                         \
        HandlerClass##Register()                                                              \
        {                                                                                     \
            foxhttp::router::register_static_handler(Path, std::make_shared<HandlerClass>()); \
        }                                                                                     \
    } HandlerClass##_register;                                                                \
    }

#define REGISTER_DYNAMIC_HANDLER(Path, HandlerClass)                                           \
    namespace {                                                                                \
    struct HandlerClass##Register                                                              \
    {                                                                                          \
        HandlerClass##Register()                                                               \
        {                                                                                      \
            foxhttp::router::register_dynamic_handler(Path, std::make_shared<HandlerClass>()); \
        }                                                                                      \
    } HandlerClass##_register;                                                                 \
    }
