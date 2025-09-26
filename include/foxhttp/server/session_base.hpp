/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * middleware utilities and helper classes
 */

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/config/configs.hpp>
#include <memory>

namespace foxhttp {

class middleware_chain;

class session_base
{
public:
    explicit session_base(const boost::asio::any_io_executor &executor, std::shared_ptr<middleware_chain> chain,
                          const session_limits &limits = {});
    virtual ~session_base() = default;

    void set_limits(const session_limits &limits);
    const session_limits &limits() const;

protected:
    // Timer controls for derived classes
    void arm_idle_timer();
    void cancel_idle_timer();

    void arm_header_timer();
    void cancel_header_timer();

    void arm_body_timer();
    void cancel_body_timer();

    void on_activity();

    virtual void on_timeout_idle();
    virtual void on_timeout_header();
    virtual void on_timeout_body();

protected:
    boost::asio::any_io_executor executor_;
    std::shared_ptr<middleware_chain> chain_;
    session_limits limits_;

    // timers
    boost::asio::steady_timer idle_timer_;
    boost::asio::steady_timer header_timer_;
    boost::asio::steady_timer body_timer_;
};

}// namespace foxhttp
