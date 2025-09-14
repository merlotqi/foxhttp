/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <foxhttp/core/request_context.hpp>
#include <foxhttp/core/session.hpp>
#include <foxhttp/handler/api_handler.hpp>
#include <foxhttp/middleware/middleware.hpp>
#include <foxhttp/middleware/middleware_chain.hpp>
#include <foxhttp/router/router.hpp>
#include <iostream>

namespace foxhttp {

Session::Session(tcp::socket socket, std::shared_ptr<MiddlewareChain> global_chain)
    : socket_(std::move(socket)), global_chain_(std::move(global_chain))
{
}

void Session::start()
{
    read_request();
}

void Session::read_request()
{
    req_ = {};
    http::async_read(socket_, buffer_, req_,
                     [self = shared_from_this()](boost::beast::error_code ec, size_t bytes_transferred) {
                         self->handle_read(ec, bytes_transferred);
                     });
}

void Session::handle_read(boost::beast::error_code ec, size_t bytes_transferred)
{
    if (ec == http::error::end_of_stream)
    {
        socket_.shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    if (ec)
    {
        std::cerr << "Read error: " << ec.message() << std::endl;
        return;
    }

    process_request();
}

void Session::process_request()
{
    res_.version(req_.version());
    res_.keep_alive(req_.keep_alive());

    std::string path = req_.target();
    if (auto pos = path.find('?'); pos != std::string::npos)
        path = path.substr(0, pos);

    RequestContext ctx(req_);

    auto handler = Router::route(path, ctx);

    if (handler)
    {
        global_chain_->execute_async(
                ctx, res_,
                [self = shared_from_this(), handler, &ctx](middleware_result result, const std::string &error_message) {
                    if (result == middleware_result::continue_)
                    {
                        handler->handle_request(ctx, self->res_);
                    }
                    else if (result == middleware_result::error)
                    {
                        self->res_.result(http::status::internal_server_error);
                        self->res_.set(http::field::content_type, "application/json");
                        self->res_.body() =
                                R"({"code":500,"message":"Internal Server Error","error":")" + error_message + R"("})";
                    }
                    else if (result == middleware_result::timeout)
                    {
                        self->res_.result(http::status::gateway_timeout);
                        self->res_.set(http::field::content_type, "application/json");
                        self->res_.body() = R"({"code":504,"message":"Gateway Timeout"})";
                    }
                    else if (result == middleware_result::stop)
                    {
                        // Middleware chain was stopped, but we still need to send a response
                        // The response should already be set by the middleware
                    }
                    self->res_.prepare_payload();
                    self->write_response();
                });
    }
    else
    {
        global_chain_->execute_async(
                ctx, res_, [self = shared_from_this()](middleware_result result, const std::string &error_message) {
                    if (result == middleware_result::continue_)
                    {
                        self->res_.result(http::status::not_found);
                        self->res_.set(http::field::content_type, "application/json");
                        self->res_.body() = R"({"code":404,"message":"Not Found"})";
                    }
                    else if (result == middleware_result::error)
                    {
                        self->res_.result(http::status::internal_server_error);
                        self->res_.set(http::field::content_type, "application/json");
                        self->res_.body() =
                                R"({"code":500,"message":"Internal Server Error","error":")" + error_message + R"("})";
                    }
                    else if (result == middleware_result::timeout)
                    {
                        self->res_.result(http::status::gateway_timeout);
                        self->res_.set(http::field::content_type, "application/json");
                        self->res_.body() = R"({"code":504,"message":"Gateway Timeout"})";
                    }
                    else if (result == middleware_result::stop)
                    {
                        // Middleware chain was stopped, but we still need to send a response
                        // The response should already be set by the middleware
                    }
                    self->res_.prepare_payload();
                    self->write_response();
                });
    }
}

void Session::write_response()
{
    auto self = shared_from_this();
    http::async_write(socket_, res_, [self](boost::beast::error_code ec, size_t) {
        if (ec)
        {
            std::cerr << "Write error: " << ec.message() << std::endl;
            return;
        }

        if (self->res_.keep_alive())
        {
            self->read_request();
        }
        else
        {
            self->socket_.shutdown(tcp::socket::shutdown_send, ec);
        }
    });
}

}// namespace foxhttp
