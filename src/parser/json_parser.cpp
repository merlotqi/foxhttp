/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <algorithm>
#include <foxhttp/parser/details/json_parser_core.hpp>
#include <foxhttp/parser/json_parser.hpp>
#include <fstream>
#include <stdexcept>

namespace foxhttp {

json_parser::json_parser(const json_config &config) : core_(std::make_unique<details::json_parser_core>())
{
    core_->config_ = config;
}

json_parser::~json_parser() = default;

std::string json_parser::name() const
{
    return "json";
}

std::string json_parser::content_type() const
{
    return "application/json";
}

bool json_parser::supports(const http::request<http::string_body> &req) const
{
    auto content_type = req[http::field::content_type];
    return content_type == "application/json" || content_type.starts_with("application/json;");
}

nlohmann::json json_parser::parse(const http::request<http::string_body> &req) const
{
    // Validate size
    if (req.body().size() > core_->config_.max_size)
    {
        throw std::runtime_error("JSON size exceeds maximum allowed size");
    }

    return core_->parse_with_validation(req.body());
}

namespace details {


nlohmann::json json_parser_core::parse_with_validation(const std::string &json_string) const
{
    // Preprocess JSON if needed
    std::string processed_json = preprocess_json(json_string);

    try
    {
        nlohmann::json json = nlohmann::json::parse(processed_json);

        // Validate depth
        auto depth_result = validate_depth(json);
        if (!depth_result.is_valid)
        {
            throw std::runtime_error("JSON depth validation failed: " + depth_result.errors[0]);
        }

        return json;
    }
    catch (const nlohmann::json::parse_error &e)
    {
        throw std::runtime_error("JSON parse error: " + std::string(e.what()));
    }
}

json_validation_result json_parser_core::validate_size(const std::string &json_string) const
{
    json_validation_result result;

    if (json_string.size() > config_.max_size)
    {
        result.add_error("JSON size (" + std::to_string(json_string.size()) + ") exceeds maximum allowed size (" +
                         std::to_string(config_.max_size) + ")");
    }

    return result;
}

json_validation_result json_parser_core::validate_depth(const nlohmann::json &json) const
{
    json_validation_result result;

    std::size_t depth = calculate_depth(json);
    if (depth > config_.max_depth)
    {
        result.add_error("JSON depth (" + std::to_string(depth) + ") exceeds maximum allowed depth (" +
                         std::to_string(config_.max_depth) + ")");
    }

    return result;
}

json_validation_result json_parser_core::validate_schema(const nlohmann::json &json) const
{
    json_validation_result result;

    // Basic schema validation (simplified)
    // In a real implementation, you'd use a proper JSON schema validator
    if (config_.schema.is_object())
    {
        // Check required fields
        if (config_.schema.contains("required") && config_.schema["required"].is_array())
        {
            for (const auto &required_field: config_.schema["required"])
            {
                if (required_field.is_string() && !json.contains(required_field.get<std::string>()))
                {
                    result.add_error("Missing required field: " + required_field.get<std::string>());
                }
            }
        }

        // Check field types
        if (config_.schema.contains("properties") && config_.schema["properties"].is_object())
        {
            for (auto &[key, value]: config_.schema["properties"].items())
            {
                if (json.contains(key) && value.is_object() && value.contains("type"))
                {
                    std::string expected_type = value["type"];
                    std::string actual_type;

                    if (json[key].is_string())
                        actual_type = "string";
                    else if (json[key].is_number())
                        actual_type = "number";
                    else if (json[key].is_boolean())
                        actual_type = "boolean";
                    else if (json[key].is_array())
                        actual_type = "array";
                    else if (json[key].is_object())
                        actual_type = "object";
                    else if (json[key].is_null())
                        actual_type = "null";

                    if (actual_type != expected_type)
                    {
                        result.add_error("Field '" + key + "' has wrong type. Expected: " + expected_type +
                                         ", Got: " + actual_type);
                    }
                }
            }
        }
    }

    return result;
}

std::size_t json_parser_core::calculate_depth(const nlohmann::json &json, std::size_t current_depth) const
{
    if (json.is_object())
    {
        std::size_t max_depth = current_depth;
        for (const auto &[key, value]: json.items())
        {
            max_depth = std::max(max_depth, calculate_depth(value, current_depth + 1));
        }
        return max_depth;
    }
    else if (json.is_array())
    {
        std::size_t max_depth = current_depth;
        for (const auto &item: json)
        {
            max_depth = std::max(max_depth, calculate_depth(item, current_depth + 1));
        }
        return max_depth;
    }

    return current_depth;
}

std::string json_parser_core::preprocess_json(const std::string &json_string) const
{
    std::string result = json_string;

    // Remove comments if not allowed
    if (!config_.allow_comments)
    {
        // Simple comment removal (not comprehensive)
        size_t pos = 0;
        while ((pos = result.find("//", pos)) != std::string::npos)
        {
            size_t end = result.find('\n', pos);
            if (end != std::string::npos)
            {
                result.erase(pos, end - pos);
            }
            else
            {
                result.erase(pos);
                break;
            }
        }
    }

    // Remove trailing commas if not allowed
    if (!config_.allow_trailing_commas)
    {
        // Simple trailing comma removal (not comprehensive)
        size_t pos = 0;
        while ((pos = result.find(",}", pos)) != std::string::npos)
        {
            result.replace(pos, 2, "}");
        }
        while ((pos = result.find(",]", pos)) != std::string::npos)
        {
            result.replace(pos, 2, "]");
        }
    }

    return result;
}

}// namespace details

}// namespace foxhttp