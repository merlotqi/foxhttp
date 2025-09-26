/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Merlot.Qi
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#pragma once

#include <foxhttp/config/configs.hpp>
#include <string>
#include <vector>

namespace foxhttp {
namespace details {

class plain_text_parser_core
{
public:
    plain_text_config config_;

    // Processing methods
    std::string process_text(const std::string &text) const;
    bool is_allowed_content_type(const std::string &content_type) const;

    // Utility methods
    std::string normalize_line_endings(const std::string &text) const;
    std::string trim_whitespace(const std::string &text) const;
    bool is_valid_utf8(const std::string &text) const;
};

}// namespace details
}// namespace foxhttp
