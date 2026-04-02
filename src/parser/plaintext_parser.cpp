#include <boost/algorithm/string.hpp>
#include <foxhttp/parser/detail/plain_text_parser_impl.hpp>
#include <foxhttp/parser/plaintext_parser.hpp>
#include <stdexcept>

namespace foxhttp::parser {

/* ---------------------------- plain_text_parser --------------------------- */

PlainTextParser::PlainTextParser(const PlainTextConfig &config)
    : core_(std::make_unique<detail::PlainTextParserImpl>()) {
  core_->config_ = config;

  // Set default allowed content types if none specified
  if (core_->config_.allowed_content_types.empty()) {
    core_->config_.allowed_content_types = {"text/plain", "text/html", "text/css",     "text/javascript",
                                            "text/xml",   "text/csv",  "text/markdown"};
  }
}

PlainTextParser::~PlainTextParser() = default;

std::string PlainTextParser::name() const { return "plaintext"; }

std::string PlainTextParser::content_type() const { return "text/plain"; }

bool PlainTextParser::supports(const http::request<http::string_body> &req) const {
  auto content_type = req[http::field::content_type];

  if (core_->config_.strict_mode) {
    return core_->is_allowed_content_type(std::string(content_type));
  } else {
    // Default behavior: accept text/* content types
    return content_type.starts_with("text/");
  }
}

std::string PlainTextParser::parse(const http::request<http::string_body> &req) const {
  // Validate size
  if (req.body().size() > core_->config_.max_size) {
    throw std::runtime_error("Text size exceeds maximum allowed size");
  }

  // Validate content type if in strict mode
  if (core_->config_.strict_mode) {
    auto content_type = req[http::field::content_type];
    if (!core_->is_allowed_content_type(std::string(content_type))) {
      throw std::runtime_error("Content type not allowed: " + std::string(content_type));
    }
  }

  return core_->process_text(req.body());
}

const PlainTextConfig &PlainTextParser::config() const { return core_->config_; }

void PlainTextParser::set_config(const PlainTextConfig &config) { core_->config_ = config; }

namespace detail {

std::string PlainTextParserImpl::process_text(const std::string &text) const {
  std::string result = text;

  // Normalize line endings
  if (config_.normalize_line_endings) {
    result = normalize_line_endings(result);
  }

  // Trim whitespace
  if (config_.trim_whitespace) {
    result = trim_whitespace(result);
  }

  // Validate encoding
  if (config_.validate_encoding && !is_valid_utf8(result)) {
    throw std::runtime_error("Invalid UTF-8 encoding detected");
  }

  return result;
}

bool PlainTextParserImpl::is_allowed_content_type(const std::string &content_type) const {
  if (config_.allowed_content_types.empty()) {
    return true;
  }

  for (const auto &allowed : config_.allowed_content_types) {
    if (content_type == allowed || boost::starts_with(content_type, allowed + ";")) {
      return true;
    }
  }

  return false;
}

std::string PlainTextParserImpl::normalize_line_endings(const std::string &text) const {
  std::string result = text;

  // Convert CRLF to LF
  size_t pos = 0;
  while ((pos = result.find("\r\n", pos)) != std::string::npos) {
    result.replace(pos, 2, "\n");
    pos += 1;
  }

  // Convert remaining CR to LF
  pos = 0;
  while ((pos = result.find('\r', pos)) != std::string::npos) {
    result.replace(pos, 1, "\n");
    pos += 1;
  }

  return result;
}

std::string PlainTextParserImpl::trim_whitespace(const std::string &text) const {
  if (text.empty()) {
    return text;
  }

  size_t start = 0;
  size_t end = text.length() - 1;

  // Find first non-whitespace character
  while (start <= end && std::isspace(text[start])) {
    start++;
  }

  // Find last non-whitespace character
  while (end >= start && std::isspace(text[end])) {
    end--;
  }

  if (start > end) {
    return "";
  }

  return text.substr(start, end - start + 1);
}

bool PlainTextParserImpl::is_valid_utf8(const std::string &text) const {
  const unsigned char *bytes = reinterpret_cast<const unsigned char *>(text.c_str());
  size_t len = text.length();

  for (size_t i = 0; i < len;) {
    unsigned char byte = bytes[i];

    if (byte < 0x80) {
      // ASCII character
      i++;
    } else if ((byte & 0xE0) == 0xC0) {
      // 2-byte character
      if (i + 1 >= len || (bytes[i + 1] & 0xC0) != 0x80) {
        return false;
      }
      i += 2;
    } else if ((byte & 0xF0) == 0xE0) {
      // 3-byte character
      if (i + 2 >= len || (bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80) {
        return false;
      }
      i += 3;
    } else if ((byte & 0xF8) == 0xF0) {
      // 4-byte character
      if (i + 3 >= len || (bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80 ||
          (bytes[i + 3] & 0xC0) != 0x80) {
        return false;
      }
      i += 4;
    } else {
      return false;
    }
  }

  return true;
}
}  // namespace detail

}  // namespace foxhttp::parser
