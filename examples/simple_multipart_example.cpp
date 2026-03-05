#include <boost/beast/http.hpp>
#include <foxhttp/foxhttp.hpp>
#include <iostream>

namespace http = boost::beast::http;
using namespace foxhttp;

int main() {
  std::cout << "FoxHttp Simplified Multipart Parser Demo\n";
  std::cout << "========================================\n\n";

  // Create a simple multipart request
  std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
  std::string body = "--" + boundary +
                     "\r\n"
                     "Content-Disposition: form-data; name=\"username\"\r\n"
                     "\r\n"
                     "john_doe\r\n"
                     "--" +
                     boundary +
                     "\r\n"
                     "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
                     "Content-Type: text/plain\r\n"
                     "\r\n"
                     "Hello, World!\r\n"
                     "--" +
                     boundary + "--\r\n";

  // Create HTTP request
  http::request<http::string_body> req;
  req.method(http::verb::post);
  req.target("/upload");
  req.set(http::field::content_type, "multipart/form-data; boundary=" + boundary);
  req.body() = body;
  req.prepare_payload();

  try {
    // Create parser with default config
    multipart_config config;
    config.max_field_size = 1024 * 1024;      // 1MB
    config.max_file_size = 10 * 1024 * 1024;  // 10MB

    multipart_parser parser(config);

    // Parse the request
    auto result = parser.parse(req);

    std::cout << "Parsed " << result.size() << " fields:\n";

    for (const auto &field : result) {
      std::cout << "\nField: " << field->name() << "\n";
      std::cout << "  Type: " << (field->is_file() ? "File" : "Text") << "\n";

      if (field->is_file()) {
        std::cout << "  Filename: " << field->filename() << "\n";
        std::cout << "  Content-Type: " << field->content_type() << "\n";
        std::cout << "  Size: " << field->size() << " bytes\n";
      }

      std::cout << "  Content: " << field->content() << "\n";
    }

    std::cout << "\nDemo completed successfully!\n";
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
