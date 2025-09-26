/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <foxhttp/middleware/middleware.hpp>

namespace foxhttp {


class compression_middleware : public priority_middleware<middleware_priority::high>
{
public:
    enum class compression_type
    {
        gzip,
        deflate,
        br,
        none,
    };

    explicit compression_middleware(size_t threshold = 1024, bool enable_gzip = true, bool enable_deflate = true,
                                    bool enable_br = true);
    std::string name() const override;
    void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next) override;
    void operator()(request_context &ctx, http::response<http::string_body> &res, std::function<void()> next,
                    async_middleware_callback callback) override;

private:
    bool _should_compress(const request_context &ctx, const http::response<http::string_body> &res);
    void _compress_response(const request_context &ctx, http::response<http::string_body> &res);
    std::string _get_encoding_name(compression_type type);
    std::string _compress_gzip(const std::string &input);
    std::string _compress_deflate(const std::string &input);
    std::string _compress_brotli(const std::string &input);

private:
    size_t threshold_;
    std::vector<compression_type> supported_types_;
};

}// namespace foxhttp
