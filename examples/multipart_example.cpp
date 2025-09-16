/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 * Advanced example usage of multipart_parser with streaming support
 */

#include <boost/beast/http.hpp>
#include <chrono>
#include <foxhttp/foxhttp.hpp>
#include <iostream>
#include <thread>

namespace http = boost::beast::http;

using namespace foxhttp;

void demonstrate_basic_parsing()
{
    std::cout << "=== Basic Multipart Parsing ===\n";

    // Example multipart/form-data request body
    std::string multipart_body =
            "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
            "Content-Disposition: form-data; name=\"username\"\r\n"
            "\r\n"
            "john_doe\r\n"
            "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
            "Content-Disposition: form-data; name=\"email\"\r\n"
            "\r\n"
            "john@example.com\r\n"
            "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
            "Content-Disposition: form-data; name=\"avatar\"; filename=\"avatar.jpg\"\r\n"
            "Content-Type: image/jpeg\r\n"
            "Content-Transfer-Encoding: base64\r\n"
            "\r\n"
            "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8/5+hHgAHggJ/PchI7wAAAABJRU5ErkJggg==\r\n"
            "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

    // Create a mock HTTP request
    http::request<http::string_body> req;
    req.set(http::field::content_type, "multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW");
    req.body() = multipart_body;

    try
    {
        // Create parser with custom configuration
        foxhttp::multipart_config config;
        config.max_field_size = 1024 * 1024;    // 1MB
        config.max_file_size = 10 * 1024 * 1024;// 10MB
        config.allowed_extensions = {".jpg", ".png", ".gif"};
        config.allowed_content_types = {"image/jpeg", "image/png", "image/gif"};

        foxhttp::multipart_parser parser(config);

        if (parser.supports(req))
        {
            std::cout << "✓ Parser supports this request\n";

            auto fields = parser.parse(req);
            std::cout << "✓ Parsed " << fields.size() << " fields:\n\n";

            for (const auto &field: fields)
            {
                std::cout << "Field: " << field->name() << "\n";
                std::cout << "  Type: " << (field->is_file() ? "File" : "Text") << "\n";
                std::cout << "  Size: " << field->size() << " bytes\n";
                std::cout << "  Valid: " << (field->is_valid() ? "Yes" : "No") << "\n";

                if (!field->is_valid())
                {
                    std::cout << "  Error: " << field->validation_error() << "\n";
                }

                if (field->is_file())
                {
                    std::cout << "  Filename: " << field->filename() << "\n";
                    std::cout << "  Content-Type: " << field->content_type() << "\n";
                    std::cout << "  Encoding: " << field->encoding() << "\n";

                    // Get content as bytes
                    auto bytes = field->content_as_bytes();
                    std::cout << "  Binary Size: " << bytes.size() << " bytes\n";
                }
                else
                {
                    std::cout << "  Value: " << field->content() << "\n";
                }

                std::cout << "  Headers:\n";
                for (const auto &[name, value]: field->headers())
                {
                    std::cout << "    " << name << ": " << value << "\n";
                }
                std::cout << "\n";
            }

            // Validate all fields
            auto errors = parser.validate_all_fields(fields);
            if (!errors.empty())
            {
                std::cout << "Validation errors:\n";
                for (const auto &error: errors)
                {
                    std::cout << "  - " << error << "\n";
                }
            }
            else
            {
                std::cout << "✓ All fields passed validation\n";
            }
        }
        else
        {
            std::cout << "✗ Parser does not support this request\n";
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void demonstrate_streaming_parsing()
{
    std::cout << "\n=== Streaming Multipart Parsing ===\n";

    // Simulate streaming data
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string full_boundary = "--" + boundary;

    std::vector<std::string> chunks = {full_boundary + "\r\n",
                                       "Content-Disposition: form-data; name=\"message\"\r\n",
                                       "\r\n",
                                       "Hello, this is a streaming upload!\r\n",
                                       full_boundary + "\r\n",
                                       "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n",
                                       "Content-Type: text/plain\r\n",
                                       "\r\n",
                                       "This is the content of the file.\n",
                                       "It has multiple lines.\n",
                                       "And some special characters: àáâãäåæçèéêë\r\n",
                                       full_boundary + "--\r\n"};

    try
    {
        // Create streaming parser with progress callback
        foxhttp::multipart_config config;
        config.max_field_size = 1024 * 1024;
        config.max_file_size = 10 * 1024 * 1024;

        auto progress_callback = [](std::size_t bytes_received, std::size_t total_bytes,
                                    const std::string &field_name) {
            std::cout << "Progress: " << bytes_received << " bytes received";
            if (!field_name.empty())
            {
                std::cout << " (field: " << field_name << ")";
            }
            std::cout << "\n";
        };

        auto error_callback = [](const std::string &error, std::size_t position) {
            std::cerr << "Streaming error at position " << position << ": " << error << "\n";
        };

        foxhttp::multipart_stream_parser stream_parser(boundary, config, progress_callback, error_callback);

        std::cout << "Starting streaming parse...\n";

        // Feed data in chunks
        for (const auto &chunk: chunks)
        {
            std::cout << "Feeding chunk: " << chunk.length() << " bytes\n";

            if (!stream_parser.feed_data(chunk))
            {
                std::cerr << "Streaming parser failed\n";
                return;
            }

            // Simulate network delay
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (stream_parser.is_complete())
        {
            std::cout << "✓ Streaming parse completed successfully\n";

            auto fields = stream_parser.get_result();
            std::cout << "✓ Parsed " << fields.size() << " fields:\n\n";

            for (const auto &field: fields)
            {
                std::cout << "Field: " << field->name() << "\n";
                std::cout << "  Type: " << (field->is_file() ? "File" : "Text") << "\n";
                std::cout << "  Size: " << field->size() << " bytes\n";

                if (field->is_file())
                {
                    std::cout << "  Filename: " << field->filename() << "\n";
                    std::cout << "  Content-Type: " << field->content_type() << "\n";
                }

                std::cout << "  Content: " << field->content().substr(0, 100);
                if (field->content().length() > 100)
                {
                    std::cout << "...";
                }
                std::cout << "\n\n";
            }
        }
        else
        {
            std::cout << "✗ Streaming parse incomplete\n";
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void demonstrate_error_handling()
{
    std::cout << "\n=== Error Handling Demonstration ===\n";

    // Test with invalid boundary
    try
    {
        http::request<http::string_body> req;
        req.set(http::field::content_type, "multipart/form-data");// No boundary
        req.body() = "some data";

        foxhttp::multipart_parser parser;
        auto fields = parser.parse(req);

        std::cout << "✗ Should have thrown exception for missing boundary\n";
    }
    catch (const std::exception &e)
    {
        std::cout << "✓ Correctly caught error: " << e.what() << "\n";
    }

    // Test with oversized content
    try
    {
        foxhttp::multipart_config config;
        config.max_total_size = 100;// Very small limit

        std::string large_body = "--boundary\r\nContent-Disposition: form-data; name=\"data\"\r\n\r\n";
        large_body.append(200, 'x');// Exceed limit
        large_body += "\r\n--boundary--\r\n";

        foxhttp::multipart_parser parser(config);
        auto fields = parser.parse_with_boundary(large_body, "boundary");

        std::cout << "✗ Should have thrown exception for oversized content\n";
    }
    catch (const std::exception &e)
    {
        std::cout << "✓ Correctly caught error: " << e.what() << "\n";
    }
}

int main()
{
    std::cout << "FoxHttp Advanced Multipart Parser Demo\n";
    std::cout << "=====================================\n\n";

    demonstrate_basic_parsing();
    demonstrate_streaming_parsing();
    demonstrate_error_handling();

    std::cout << "\nDemo completed successfully!\n";
    return 0;
}
