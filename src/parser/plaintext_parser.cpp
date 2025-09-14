/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <foxhttp/parser/plaintext_parser.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
namespace foxhttp {

// ============================================================================
// PlainTextParser Implementation
// ============================================================================

class PlainTextParser::Impl
{
public:
    PlainTextConfig config_;

    // Processing methods
    std::string process_text(const std::string &text) const;
    bool is_allowed_content_type(const std::string &content_type) const;

    // Utility methods
    std::string normalize_line_endings(const std::string &text) const;
    std::string trim_whitespace(const std::string &text) const;
    bool is_valid_utf8(const std::string &text) const;
};

PlainTextParser::PlainTextParser(const PlainTextConfig &config) : pimpl_(std::make_unique<Impl>())
{
    pimpl_->config_ = config;

    // Set default allowed content types if none specified
    if (pimpl_->config_.allowed_content_types.empty())
    {
        pimpl_->config_.allowed_content_types = {"text/plain", "text/html", "text/css",     "text/javascript",
                                                 "text/xml",   "text/csv",  "text/markdown"};
    }
}

PlainTextParser::~PlainTextParser() = default;

std::string PlainTextParser::name() const
{
    return "plaintext";
}

std::string PlainTextParser::content_type() const
{
    return "text/plain";
}

bool PlainTextParser::supports(const http::request<http::string_body> &req) const
{
    auto content_type = req[http::field::content_type];

    if (pimpl_->config_.strict_mode)
    {
        return pimpl_->is_allowed_content_type(std::string(content_type));
    }
    else
    {
        // Default behavior: accept text/* content types
        return content_type.starts_with("text/");
    }
}

std::string PlainTextParser::parse(const http::request<http::string_body> &req) const
{
    // Validate size
    if (req.body().size() > pimpl_->config_.max_size)
    {
        throw std::runtime_error("Text size exceeds maximum allowed size");
    }

    // Validate content type if in strict mode
    if (pimpl_->config_.strict_mode)
    {
        auto content_type = req[http::field::content_type];
        if (!pimpl_->is_allowed_content_type(std::string(content_type)))
        {
            throw std::runtime_error("Content type not allowed: " + std::string(content_type));
        }
    }

    return pimpl_->process_text(req.body());
}

// parse_string and parse_file moved to private implementation

const PlainTextConfig &PlainTextParser::config() const
{
    return pimpl_->config_;
}

void PlainTextParser::set_config(const PlainTextConfig &config)
{
    pimpl_->config_ = config;
}

// All validation and utility methods moved to private implementation

// ============================================================================
// PlainTextParser::Impl Implementation
// ============================================================================

std::string PlainTextParser::Impl::process_text(const std::string &text) const
{
    std::string result = text;

    // Normalize line endings
    if (config_.normalize_line_endings)
    {
        result = normalize_line_endings(result);
    }

    // Trim whitespace
    if (config_.trim_whitespace)
    {
        result = trim_whitespace(result);
    }

    // Validate encoding
    if (config_.validate_encoding && !is_valid_utf8(result))
    {
        throw std::runtime_error("Invalid UTF-8 encoding detected");
    }

    return result;
}

bool PlainTextParser::Impl::is_allowed_content_type(const std::string &content_type) const
{
    if (config_.allowed_content_types.empty())
    {
        return true;
    }

    for (const auto &allowed: config_.allowed_content_types)
    {
        if (content_type == allowed || boost::starts_with(content_type, allowed + ";"))
        {
            return true;
        }
    }

    return false;
}

std::string PlainTextParser::Impl::normalize_line_endings(const std::string &text) const
{
    std::string result = text;

    // Convert CRLF to LF
    size_t pos = 0;
    while ((pos = result.find("\r\n", pos)) != std::string::npos)
    {
        result.replace(pos, 2, "\n");
        pos += 1;
    }

    // Convert remaining CR to LF
    pos = 0;
    while ((pos = result.find('\r', pos)) != std::string::npos)
    {
        result.replace(pos, 1, "\n");
        pos += 1;
    }

    return result;
}

std::string PlainTextParser::Impl::trim_whitespace(const std::string &text) const
{
    if (text.empty())
    {
        return text;
    }

    size_t start = 0;
    size_t end = text.length() - 1;

    // Find first non-whitespace character
    while (start <= end && std::isspace(text[start]))
    {
        start++;
    }

    // Find last non-whitespace character
    while (end >= start && std::isspace(text[end]))
    {
        end--;
    }

    if (start > end)
    {
        return "";
    }

    return text.substr(start, end - start + 1);
}

bool PlainTextParser::Impl::is_valid_utf8(const std::string &text) const
{
    const unsigned char *bytes = reinterpret_cast<const unsigned char *>(text.c_str());
    size_t len = text.length();

    for (size_t i = 0; i < len;)
    {
        unsigned char byte = bytes[i];

        if (byte < 0x80)
        {
            // ASCII character
            i++;
        }
        else if ((byte & 0xE0) == 0xC0)
        {
            // 2-byte character
            if (i + 1 >= len || (bytes[i + 1] & 0xC0) != 0x80)
            {
                return false;
            }
            i += 2;
        }
        else if ((byte & 0xF0) == 0xE0)
        {
            // 3-byte character
            if (i + 2 >= len || (bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80)
            {
                return false;
            }
            i += 3;
        }
        else if ((byte & 0xF8) == 0xF0)
        {
            // 4-byte character
            if (i + 3 >= len || (bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80 ||
                (bytes[i + 3] & 0xC0) != 0x80)
            {
                return false;
            }
            i += 4;
        }
        else
        {
            return false;
        }
    }

    return true;
}

}// namespace foxhttp
