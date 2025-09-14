#include <boost/beast/http.hpp>
#include <foxhttp/foxhttp.hpp>
#include <iomanip>
#include <iostream>

namespace http = boost::beast::http;

using namespace foxhttp;

void demonstrate_form_parser()
{
    std::cout << "=== Enhanced Form Parser Demo ===\n";

    // Create form data with arrays
    std::string form_data = "username=john_doe&"
                            "email=john@example.com&"
                            "hobbies[]=reading&"
                            "hobbies[]=coding&"
                            "hobbies[]=gaming&"
                            "age=25&"
                            "city=New%20York";

    // Create HTTP request
    http::request<http::string_body> req;
    req.method(http::verb::post);
    req.target("/submit");
    req.set(http::field::content_type, "application/x-www-form-urlencoded");
    req.body() = form_data;
    req.prepare_payload();

    try
    {
        // Configure parser
        FormConfig config;
        config.max_field_size = 1024;
        config.max_total_size = 10240;
        config.support_arrays = true;

        FormParser parser(config);
        auto result = parser.parse(req);

        std::cout << "Parsed " << result.size() << " fields:\n";

        for (const auto &[name, field]: result)
        {
            std::cout << "  " << name << ": ";
            if (field->is_array())
            {
                std::cout << "[";
                for (size_t i = 0; i < field->size(); ++i)
                {
                    if (i > 0)
                        std::cout << ", ";
                    std::cout << "\"" << field->value_at(i) << "\"";
                }
                std::cout << "]";
            }
            else
            {
                std::cout << "\"" << field->value() << "\"";
            }
            std::cout << "\n";
        }

        // Validation is now handled internally
        std::cout << "✓ Form data parsed successfully\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    std::cout << "\n";
}

void demonstrate_json_parser()
{
    std::cout << "=== Enhanced JSON Parser Demo ===\n";

    // Create JSON data
    std::string json_data = R"({
        "name": "John Doe",
        "age": 25,
        "email": "john@example.com",
        "address": {
            "street": "123 Main St",
            "city": "New York",
            "zip": "10001"
        },
        "hobbies": ["reading", "coding", "gaming"],
        "active": true
    })";

    // Create HTTP request
    http::request<http::string_body> req;
    req.method(http::verb::post);
    req.target("/api/user");
    req.set(http::field::content_type, "application/json");
    req.body() = json_data;
    req.prepare_payload();

    try
    {
        // Configure parser
        JsonConfig config;
        config.max_size = 1024 * 1024;// 1MB
        config.max_depth = 10;
        config.strict_mode = true;

        // Set up schema validation through config
        nlohmann::json schema = {
                {      "type","object"                              },
                {  "required", {"name", "age", "email"}},
                {"properties",
                 {{"name", {"type", "string"}},
                 {"age", {"type", "number"}},
                 {"email", {"type", "string"}},
                 {"address", {"type", "object"}},
                 {"hobbies", {"type", "array"}},
                 {"active", {"type", "boolean"}}}      }
        };

        config.schema = schema;
        config.validate_schema = true;

        JsonParser parser(config);

        // Parse JSON (validation is handled internally)
        auto json = parser.parse(req);

        std::cout << "✓ JSON parsed and validated successfully\n";
        std::cout << "Pretty formatted JSON:\n";
        std::cout << json.dump(2) << "\n";

        // Access some values
        std::cout << "Name: " << json["name"].get<std::string>() << "\n";
        std::cout << "Age: " << json["age"].get<int>() << "\n";
        std::cout << "Hobbies: ";
        for (const auto &hobby: json["hobbies"])
        {
            std::cout << hobby.get<std::string>() << " ";
        }
        std::cout << "\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    std::cout << "\n";
}

void demonstrate_plaintext_parser()
{
    std::cout << "=== Enhanced Plain Text Parser Demo ===\n";

    // Create text data with mixed line endings
    std::string text_data = "Hello World!\r\n"
                            "This is a test file.\r\n"
                            "It has mixed line endings.\n"
                            "And some trailing spaces   \n"
                            "  And leading spaces\n";

    // Create HTTP request
    http::request<http::string_body> req;
    req.method(http::verb::post);
    req.target("/upload");
    req.set(http::field::content_type, "text/plain");
    req.body() = text_data;
    req.prepare_payload();

    try
    {
        // Configure parser
        PlainTextConfig config;
        config.max_size = 1024 * 1024;// 1MB
        config.normalize_line_endings = true;
        config.trim_whitespace = true;
        config.validate_encoding = true;
        config.strict_mode = false;// Allow various text/* types

        PlainTextParser parser(config);
        auto result = parser.parse(req);

        std::cout << "✓ Text parsed successfully\n";
        std::cout << "Original size: " << text_data.size() << " bytes\n";
        std::cout << "Processed size: " << result.size() << " bytes\n";

        // Show processed result
        std::cout << "Processed text:\n";
        std::cout << "\"" << result << "\"\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    std::cout << "\n";
}

void demonstrate_error_handling()
{
    std::cout << "=== Error Handling Demo ===\n";

    // Test oversized JSON
    std::string large_json = "{";
    for (int i = 0; i < 10000; ++i)
    {
        large_json += "\"field" + std::to_string(i) + "\": \"value" + std::to_string(i) + "\",";
    }
    large_json += "\"end\": \"true\"}";

    http::request<http::string_body> req;
    req.method(http::verb::post);
    req.target("/api/data");
    req.set(http::field::content_type, "application/json");
    req.body() = large_json;
    req.prepare_payload();

    try
    {
        JsonConfig config;
        config.max_size = 1024;// Very small limit

        JsonParser parser(config);
        auto json = parser.parse(req);

        std::cout << "✗ Should have failed but didn't\n";
    }
    catch (const std::exception &e)
    {
        std::cout << "✓ Caught expected error: " << e.what() << "\n";
    }

    // Test invalid JSON
    std::string invalid_json = R"({"name": "John", "age": 25, "invalid": })";

    req.body() = invalid_json;
    req.prepare_payload();

    try
    {
        JsonParser parser;
        auto json = parser.parse(req);

        std::cout << "✗ Should have failed but didn't\n";
    }
    catch (const std::exception &e)
    {
        std::cout << "✓ Caught expected error: " << e.what() << "\n";
    }

    std::cout << "\n";
}

int main()
{
    std::cout << "FoxHttp Enhanced Parsers Demo\n";
    std::cout << "=============================\n\n";

    demonstrate_form_parser();
    demonstrate_json_parser();
    demonstrate_plaintext_parser();
    demonstrate_error_handling();

    std::cout << "Demo completed successfully!\n";
    return 0;
}
