/**
 * foxhttp - lightweight async HTTP server (Boost.Asio)
 * Copyright (C) 2025 Rain Merlot
 * Licensed under GPLv3: https://www.gnu.org/licenses/
 *
 */

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <cctype>
#include <filesystem>
#include <foxhttp/parser/multipart_parser.hpp>
#include <fstream>
#include <iomanip>
#include <optional>
#include <random>
#include <sstream>

namespace foxhttp {

// ============================================================================
// MultipartField Implementation
// ============================================================================

class MultipartField::Impl
{
public:
    std::string name_;
    std::string filename_;
    std::string content_type_;
    std::string encoding_;
    std::unordered_map<std::string, std::string> headers_;

    // Content storage - either in memory or temp file
    mutable std::string content_;
    mutable std::vector<uint8_t> binary_content_;
    mutable std::optional<std::string> temp_file_;
    mutable bool content_loaded_ = false;

    void load_content() const
    {
        if (content_loaded_)
        {
            return;
        }

        if (temp_file_.has_value())
        {
            std::ifstream file(temp_file_.value(), std::ios::binary);
            if (file)
            {
                std::ostringstream ss;
                ss << file.rdbuf();
                content_ = ss.str();
            }
        }

        content_loaded_ = true;
    }

    std::string to_lower(const std::string &str) const
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
};

MultipartField::MultipartField() : pimpl_(std::make_unique<Impl>()) {}

MultipartField::~MultipartField() = default;

std::size_t MultipartField::size() const
{
    if (pimpl_->temp_file_.has_value())
    {
        try
        {
            return std::filesystem::file_size(pimpl_->temp_file_.value());
        }
        catch (...)
        {
            return 0;
        }
    }
    return pimpl_->content_.size() + pimpl_->binary_content_.size();
}

const std::string &MultipartField::name() const
{
    return pimpl_->name_;
}

const std::string &MultipartField::filename() const
{
    return pimpl_->filename_;
}

const std::string &MultipartField::content_type() const
{
    return pimpl_->content_type_;
}

const std::string &MultipartField::encoding() const
{
    return pimpl_->encoding_;
}

const std::string &MultipartField::content() const
{
    pimpl_->load_content();
    return pimpl_->content_;
}

std::string MultipartField::content_as_string() const
{
    if (pimpl_->temp_file_.has_value())
    {
        std::ifstream file(pimpl_->temp_file_.value(), std::ios::binary);
        if (file)
        {
            std::ostringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }
        return "";
    }
    return pimpl_->content_;
}

std::vector<uint8_t> MultipartField::content_as_bytes() const
{
    if (pimpl_->temp_file_.has_value())
    {
        std::ifstream file(pimpl_->temp_file_.value(), std::ios::binary);
        if (file)
        {
            file.seekg(0, std::ios::end);
            std::size_t size = file.tellg();
            file.seekg(0, std::ios::beg);

            std::vector<uint8_t> data(size);
            file.read(reinterpret_cast<char *>(data.data()), size);
            return data;
        }
        return {};
    }

    if (!pimpl_->binary_content_.empty())
    {
        return pimpl_->binary_content_;
    }

    // Convert string content to bytes
    std::vector<uint8_t> data(pimpl_->content_.begin(), pimpl_->content_.end());
    return data;
}

bool MultipartField::is_file() const
{
    return !pimpl_->filename_.empty();
}

bool MultipartField::is_text() const
{
    return pimpl_->filename_.empty();
}

bool MultipartField::is_temporary() const
{
    return pimpl_->temp_file_.has_value();
}

const std::string &MultipartField::temp_file_path() const
{
    static const std::string empty_string;
    if (!pimpl_->temp_file_.has_value())
    {
        return empty_string;
    }
    return pimpl_->temp_file_.value();
}

const std::unordered_map<std::string, std::string> &MultipartField::headers() const
{
    return pimpl_->headers_;
}

std::string MultipartField::header(const std::string &name) const
{
    auto it = pimpl_->headers_.find(pimpl_->to_lower(name));
    return it != pimpl_->headers_.end() ? it->second : "";
}

bool MultipartField::has_header(const std::string &name) const
{
    return pimpl_->headers_.find(pimpl_->to_lower(name)) != pimpl_->headers_.end();
}

bool MultipartField::is_valid() const
{
    return !pimpl_->name_.empty() && validation_error().empty();
}

std::string MultipartField::validation_error() const
{
    if (pimpl_->name_.empty())
    {
        return "Field name is empty";
    }

    if (is_file() && pimpl_->filename_.empty())
    {
        return "File field has no filename";
    }

    return "";
}

void MultipartField::set_name(const std::string &name)
{
    pimpl_->name_ = name;
}

void MultipartField::set_filename(const std::string &filename)
{
    pimpl_->filename_ = filename;
}

void MultipartField::set_content_type(const std::string &content_type)
{
    pimpl_->content_type_ = content_type;
}

void MultipartField::set_encoding(const std::string &encoding)
{
    pimpl_->encoding_ = encoding;
}

void MultipartField::add_header(const std::string &name, const std::string &value)
{
    pimpl_->headers_[pimpl_->to_lower(name)] = value;
}

void MultipartField::set_content(const std::string &content)
{
    pimpl_->content_ = content;
    pimpl_->binary_content_.clear();
    pimpl_->temp_file_.reset();
    pimpl_->content_loaded_ = true;
}

void MultipartField::set_content(std::vector<uint8_t> content)
{
    pimpl_->binary_content_ = std::move(content);
    pimpl_->content_.clear();
    pimpl_->temp_file_.reset();
    pimpl_->content_loaded_ = true;
}

void MultipartField::set_temp_file(const std::string &path)
{
    pimpl_->temp_file_ = path;
    pimpl_->content_.clear();
    pimpl_->binary_content_.clear();
    pimpl_->content_loaded_ = true;
}

void MultipartField::cleanup_temp_file()
{
    if (pimpl_->temp_file_.has_value())
    {
        try
        {
            std::filesystem::remove(pimpl_->temp_file_.value());
        }
        catch (...)
        {
            // Ignore cleanup errors
        }
        pimpl_->temp_file_.reset();
    }
}

// ============================================================================
// MultipartParser Implementation
// ============================================================================

class MultipartParser::Impl
{
public:
    MultipartConfig config_;

    // Parsing methods
    std::string extract_boundary(const std::string &content_type) const;
    std::unique_ptr<MultipartField> parse_field(const std::string &field_data) const;
    std::unordered_map<std::string, std::string> parse_headers(const std::string &header_data) const;
    std::string decode_content(const std::string &content, const std::string &encoding) const;
    std::vector<uint8_t> decode_binary_content(const std::string &content, const std::string &encoding) const;

    // Utility methods
    std::string trim(const std::string &str) const;
    std::string to_lower(const std::string &str) const;
    std::string generate_temp_filename() const;
    bool is_allowed_extension(const std::string &filename) const;
    bool is_allowed_content_type(const std::string &content_type) const;

    // Base64 decoding
    std::vector<uint8_t> base64_decode(const std::string &encoded) const;
    std::string base64_decode_string(const std::string &encoded) const;

    // Quoted-printable decoding
    std::string quoted_printable_decode(const std::string &encoded) const;
};

MultipartParser::MultipartParser(const MultipartConfig &config) : pimpl_(std::make_unique<Impl>())
{
    pimpl_->config_ = config;
}

MultipartParser::~MultipartParser() = default;

std::string MultipartParser::name() const
{
    return "multipart";
}

std::string MultipartParser::content_type() const
{
    return "multipart/form-data";
}

bool MultipartParser::supports(const http::request<http::string_body> &req) const
{
    auto content_type = req[http::field::content_type];
    return content_type.starts_with("multipart/form-data");
}

MultipartData MultipartParser::parse(const http::request<http::string_body> &req) const
{
    auto content_type = req[http::field::content_type];
    std::string boundary = pimpl_->extract_boundary(std::string(content_type));

    if (boundary.empty())
    {
        throw std::runtime_error("No boundary found in multipart content-type");
    }

    return parse_with_boundary(req.body(), boundary);
}

MultipartData MultipartParser::parse_with_boundary(const std::string &body, const std::string &boundary) const
{
    MultipartData result;

    // Validate total size
    if (body.size() > pimpl_->config_.max_total_size)
    {
        throw std::runtime_error("Request body exceeds maximum total size");
    }

    // The boundary should be prefixed with "--"
    std::string full_boundary = "--" + boundary;
    std::string end_boundary = "--" + boundary + "--";

    size_t pos = 0;
    std::size_t total_processed = 0;

    while (pos < body.length())
    {
        // Find the start of the next field
        size_t field_start = body.find(full_boundary, pos);
        if (field_start == std::string::npos)
        {
            break;
        }

        // Move past the boundary
        field_start += full_boundary.length();

        // Check if this is the end boundary
        if (body.substr(field_start, 2) == "--")
        {
            break;
        }

        // Find the end of this field (next boundary)
        size_t field_end = body.find(full_boundary, field_start);
        if (field_end == std::string::npos)
        {
            // Last field, find end boundary
            field_end = body.find(end_boundary, field_start);
            if (field_end == std::string::npos)
            {
                break;
            }
        }

        // Extract field data (skip CRLF after boundary)
        if (body.substr(field_start, 2) == "\r\n")
        {
            field_start += 2;
        }
        else if (body.substr(field_start, 1) == "\n")
        {
            field_start += 1;
        }

        // Remove trailing CRLF before next boundary
        while (field_end > field_start && (body[field_end - 1] == '\n' || body[field_end - 1] == '\r'))
        {
            field_end--;
        }

        std::string field_data = body.substr(field_start, field_end - field_start);

        if (!field_data.empty())
        {
            auto field = pimpl_->parse_field(field_data);
            if (field && !field->name().empty())
            {
                // Validate field
                if (validate_field(*field))
                {
                    result.push_back(std::move(field));
                }
                else
                {
                    throw std::runtime_error("Field validation failed: " + field->validation_error());
                }
            }
        }

        total_processed += field_end - pos;
        pos = field_end;
    }

    return result;
}

std::unique_ptr<MultipartStreamParser> MultipartParser::create_stream_parser(const std::string &boundary,
                                                                             ProgressCallback progress_cb,
                                                                             ErrorCallback error_cb) const
{
    return std::make_unique<MultipartStreamParser>(boundary, pimpl_->config_, progress_cb, error_cb);
}

const MultipartConfig &MultipartParser::config() const
{
    return pimpl_->config_;
}

void MultipartParser::set_config(const MultipartConfig &config)
{
    pimpl_->config_ = config;
}

bool MultipartParser::validate_field(const MultipartField &field) const
{
    if (!field.is_valid())
    {
        return false;
    }

    // Check field size limits
    if (field.is_file())
    {
        if (field.size() > pimpl_->config_.max_file_size)
        {
            return false;
        }

        // Check file extension
        if (!pimpl_->config_.allowed_extensions.empty())
        {
            std::string ext = std::filesystem::path(field.filename()).extension().string();
            if (std::find(pimpl_->config_.allowed_extensions.begin(), pimpl_->config_.allowed_extensions.end(), ext) ==
                pimpl_->config_.allowed_extensions.end())
            {
                return false;
            }
        }

        // Check content type
        if (!pimpl_->config_.allowed_content_types.empty() && !field.content_type().empty())
        {
            if (std::find(pimpl_->config_.allowed_content_types.begin(), pimpl_->config_.allowed_content_types.end(),
                          field.content_type()) == pimpl_->config_.allowed_content_types.end())
            {
                return false;
            }
        }
    }
    else
    {
        if (field.size() > pimpl_->config_.max_field_size)
        {
            return false;
        }
    }

    return true;
}

std::vector<std::string> MultipartParser::validate_all_fields(const MultipartData &data) const
{
    std::vector<std::string> errors;

    for (const auto &field: data)
    {
        if (!validate_field(*field))
        {
            errors.push_back("Field '" + field->name() + "': " + field->validation_error());
        }
    }

    return errors;
}

std::string MultipartParser::Impl::extract_boundary(const std::string &content_type) const
{
    // Look for boundary parameter in Content-Type header
    // Format: multipart/form-data; boundary=----WebKitFormBoundary...

    size_t boundary_pos = content_type.find("boundary=");
    if (boundary_pos == std::string::npos)
    {
        return "";
    }

    boundary_pos += 9;// Length of "boundary="

    // Find the end of the boundary value
    size_t end_pos = content_type.find_first_of("; \t\r\n", boundary_pos);
    if (end_pos == std::string::npos)
    {
        end_pos = content_type.length();
    }

    std::string boundary = content_type.substr(boundary_pos, end_pos - boundary_pos);

    // Remove quotes if present
    if (boundary.length() >= 2 && boundary.front() == '"' && boundary.back() == '"')
    {
        boundary = boundary.substr(1, boundary.length() - 2);
    }

    return boundary;
}

std::unique_ptr<MultipartField> MultipartParser::Impl::parse_field(const std::string &field_data) const
{
    auto field = std::make_unique<MultipartField>();

    // Find the double CRLF that separates headers from content
    size_t header_end = field_data.find("\r\n\r\n");
    if (header_end == std::string::npos)
    {
        header_end = field_data.find("\n\n");
        if (header_end == std::string::npos)
        {
            return nullptr;// Invalid field format
        }
        header_end += 2;
    }
    else
    {
        header_end += 4;
    }

    // Parse headers
    std::string header_data = field_data.substr(0, header_end - 4);
    auto headers = parse_headers(header_data);

    // Set headers
    for (const auto &[name, value]: headers)
    {
        field->add_header(name, value);
    }

    // Extract content
    std::string content = field_data.substr(header_end);

    // Parse Content-Disposition header to get field name and filename
    auto disposition_it = headers.find("content-disposition");
    if (disposition_it != headers.end())
    {
        std::string disposition = disposition_it->second;

        // Extract name parameter
        size_t name_pos = disposition.find("name=\"");
        if (name_pos != std::string::npos)
        {
            name_pos += 6;// Length of "name=\""
            size_t name_end = disposition.find("\"", name_pos);
            if (name_end != std::string::npos)
            {
                field->set_name(disposition.substr(name_pos, name_end - name_pos));
            }
        }

        // Extract filename parameter
        size_t filename_pos = disposition.find("filename=\"");
        if (filename_pos != std::string::npos)
        {
            filename_pos += 10;// Length of "filename=\""
            size_t filename_end = disposition.find("\"", filename_pos);
            if (filename_end != std::string::npos)
            {
                field->set_filename(disposition.substr(filename_pos, filename_end - filename_pos));
            }
        }
    }

    // Get Content-Type
    auto content_type_it = headers.find("content-type");
    if (content_type_it != headers.end())
    {
        field->set_content_type(content_type_it->second);
    }

    // Get encoding
    auto encoding_it = headers.find("content-transfer-encoding");
    if (encoding_it != headers.end())
    {
        field->set_encoding(encoding_it->second);
    }

    // Decode content if needed
    if (!field->encoding().empty())
    {
        if (field->is_file())
        {
            auto decoded = decode_binary_content(content, field->encoding());
            field->set_content(std::move(decoded));
        }
        else
        {
            std::string decoded = decode_content(content, field->encoding());
            field->set_content(decoded);
        }
    }
    else
    {
        field->set_content(content);
    }

    return field;
}

std::unordered_map<std::string, std::string> MultipartParser::Impl::parse_headers(const std::string &header_data) const
{
    std::unordered_map<std::string, std::string> headers;

    std::istringstream stream(header_data);
    std::string line;

    while (std::getline(stream, line))
    {
        // Remove CR if present
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos)
        {
            std::string name = trim(line.substr(0, colon_pos));
            std::string value = trim(line.substr(colon_pos + 1));

            headers[to_lower(name)] = value;
        }
    }

    return headers;
}

std::string MultipartParser::Impl::decode_content(const std::string &content, const std::string &encoding) const
{
    std::string lower_encoding = to_lower(encoding);

    if (lower_encoding == "quoted-printable")
    {
        return quoted_printable_decode(content);
    }
    else if (lower_encoding == "base64")
    {
        return base64_decode_string(content);
    }

    // No encoding or unknown encoding, return as-is
    return content;
}

std::vector<uint8_t> MultipartParser::Impl::decode_binary_content(const std::string &content,
                                                                  const std::string &encoding) const
{
    std::string lower_encoding = to_lower(encoding);

    if (lower_encoding == "base64")
    {
        return base64_decode(content);
    }

    // For other encodings, convert string to bytes
    std::string decoded = decode_content(content, encoding);
    return std::vector<uint8_t>(decoded.begin(), decoded.end());
}

std::string MultipartParser::Impl::trim(const std::string &str) const
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
    {
        return "";
    }

    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string MultipartParser::Impl::to_lower(const std::string &str) const
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string MultipartParser::Impl::generate_temp_filename() const
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);

    std::string filename = config_.temp_directory + "/foxhttp_upload_";

    for (int i = 0; i < 16; ++i)
    {
        filename += "0123456789abcdef"[dis(gen)];
    }

    return filename;
}

bool MultipartParser::Impl::is_allowed_extension(const std::string &filename) const
{
    if (config_.allowed_extensions.empty())
    {
        return true;
    }

    std::string ext = std::filesystem::path(filename).extension().string();
    return std::find(config_.allowed_extensions.begin(), config_.allowed_extensions.end(), ext) !=
           config_.allowed_extensions.end();
}

bool MultipartParser::Impl::is_allowed_content_type(const std::string &content_type) const
{
    if (config_.allowed_content_types.empty())
    {
        return true;
    }

    return std::find(config_.allowed_content_types.begin(), config_.allowed_content_types.end(), content_type) !=
           config_.allowed_content_types.end();
}

std::vector<uint8_t> MultipartParser::Impl::base64_decode(const std::string &encoded) const
{
    // Simple base64 decoding implementation
    // In production, use a proper base64 library
    std::vector<uint8_t> result;
    result.reserve((encoded.length() * 3) / 4);

    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::unordered_map<char, int> char_to_index;

    for (int i = 0; i < 64; ++i)
    {
        char_to_index[chars[i]] = i;
    }

    int val = 0, valb = -8;
    for (char c: encoded)
    {
        if (char_to_index.find(c) == char_to_index.end())
            break;
        val = (val << 6) + char_to_index[c];
        valb += 6;
        if (valb >= 0)
        {
            result.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }

    return result;
}

std::string MultipartParser::Impl::base64_decode_string(const std::string &encoded) const
{
    auto bytes = base64_decode(encoded);
    return std::string(bytes.begin(), bytes.end());
}

std::string MultipartParser::Impl::quoted_printable_decode(const std::string &encoded) const
{
    std::string result;
    result.reserve(encoded.length());

    for (size_t i = 0; i < encoded.length(); ++i)
    {
        if (encoded[i] == '=' && i + 2 < encoded.length())
        {
            // Decode hex sequence
            std::string hex = encoded.substr(i + 1, 2);
            if (std::all_of(hex.begin(), hex.end(), ::isxdigit))
            {
                int value = std::stoi(hex, nullptr, 16);
                result += static_cast<char>(value);
                i += 2;
            }
            else
            {
                result += encoded[i];
            }
        }
        else if (encoded[i] == '=' && i + 1 == encoded.length())
        {
            // Soft line break, ignore
            break;
        }
        else
        {
            result += encoded[i];
        }
    }

    return result;
}

// ============================================================================
// MultipartStreamParser Implementation
// ============================================================================

class MultipartStreamParser::Impl
{
public:
    enum class ParseState
    {
        BOUNDARY,
        HEADERS,
        CONTENT,
        COMPLETE,
        error
    };

    MultipartConfig config_;
    std::string boundary_;
    std::string full_boundary_;
    std::string end_boundary_;

    ParseState state_;
    std::size_t bytes_processed_;
    std::size_t current_field_size_;
    std::string current_field_name_;

    std::string buffer_;
    std::unique_ptr<MultipartField> current_field_;
    MultipartData result_;

    ProgressCallback progress_callback_;
    ErrorCallback error_callback_;

    // Parsing methods
    bool process_boundary();
    bool process_headers();
    bool process_content();
    void finalize_current_field();
    void handle_error(const std::string &error);

    // Content handling
    void write_content_data(const char *data, std::size_t size);
    void switch_to_temp_file();

    // Utility
    std::string generate_temp_filename() const;
    void cleanup_temp_files();
};

MultipartStreamParser::MultipartStreamParser(const std::string &boundary, const MultipartConfig &config,
                                             ProgressCallback progress_cb, ErrorCallback error_cb)
    : pimpl_(std::make_unique<Impl>())
{
    pimpl_->config_ = config;
    pimpl_->boundary_ = boundary;
    pimpl_->full_boundary_ = "--" + boundary;
    pimpl_->end_boundary_ = "--" + boundary + "--";
    pimpl_->state_ = Impl::ParseState::BOUNDARY;
    pimpl_->bytes_processed_ = 0;
    pimpl_->current_field_size_ = 0;
    pimpl_->progress_callback_ = progress_cb;
    pimpl_->error_callback_ = error_cb;
}

MultipartStreamParser::~MultipartStreamParser() = default;

bool MultipartStreamParser::feed_data(const char *data, std::size_t size)
{
    pimpl_->buffer_.append(data, size);
    pimpl_->bytes_processed_ += size;

    while (!pimpl_->buffer_.empty() && pimpl_->state_ != Impl::ParseState::COMPLETE &&
           pimpl_->state_ != Impl::ParseState::error)
    {
        bool processed = false;

        switch (pimpl_->state_)
        {
            case Impl::ParseState::BOUNDARY:
                processed = pimpl_->process_boundary();
                break;
            case Impl::ParseState::HEADERS:
                processed = pimpl_->process_headers();
                break;
            case Impl::ParseState::CONTENT:
                processed = pimpl_->process_content();
                break;
            default:
                break;
        }

        if (!processed)
        {
            break;
        }
    }

    return pimpl_->state_ != Impl::ParseState::error;
}

bool MultipartStreamParser::feed_data(const std::string &data)
{
    return feed_data(data.data(), data.size());
}

bool MultipartStreamParser::is_complete() const
{
    return pimpl_->state_ == Impl::ParseState::COMPLETE;
}

MultipartData MultipartStreamParser::get_result()
{
    return std::move(pimpl_->result_);
}

void MultipartStreamParser::set_progress_callback(ProgressCallback callback)
{
    pimpl_->progress_callback_ = callback;
}

void MultipartStreamParser::set_error_callback(ErrorCallback callback)
{
    pimpl_->error_callback_ = callback;
}

std::size_t MultipartStreamParser::bytes_processed() const
{
    return pimpl_->bytes_processed_;
}

std::size_t MultipartStreamParser::current_field_size() const
{
    return pimpl_->current_field_size_;
}

const std::string &MultipartStreamParser::current_field_name() const
{
    return pimpl_->current_field_name_;
}

void MultipartStreamParser::reset()
{
    pimpl_->state_ = Impl::ParseState::BOUNDARY;
    pimpl_->bytes_processed_ = 0;
    pimpl_->current_field_size_ = 0;
    pimpl_->current_field_name_.clear();
    pimpl_->buffer_.clear();
    pimpl_->current_field_.reset();
    pimpl_->result_.clear();
    pimpl_->cleanup_temp_files();
}

void MultipartStreamParser::abort()
{
    pimpl_->state_ = Impl::ParseState::error;
    pimpl_->cleanup_temp_files();
}

bool MultipartStreamParser::Impl::process_boundary()
{
    // Look for boundary in buffer
    size_t boundary_pos = buffer_.find(full_boundary_);
    if (boundary_pos == std::string::npos)
    {
        // Check if we have enough data to contain a boundary
        if (buffer_.length() > full_boundary_.length() + 100)
        {
            handle_error("Boundary not found in expected position");
            return false;
        }
        return false;// Need more data
    }

    // Remove data before boundary
    buffer_.erase(0, boundary_pos + full_boundary_.length());

    // Check if this is the end boundary
    if (boost::starts_with(buffer_, "--"))
    {
        state_ = ParseState::COMPLETE;
        return true;
    }

    // Skip CRLF after boundary
    if (boost::starts_with(buffer_, "\r\n"))
    {
        buffer_.erase(0, 2);
    }
    else if (boost::starts_with(buffer_, "\n"))
    {
        buffer_.erase(0, 1);
    }

    state_ = ParseState::HEADERS;
    return true;
}

bool MultipartStreamParser::Impl::process_headers()
{
    // Look for double CRLF that ends headers
    size_t header_end = buffer_.find("\r\n\r\n");
    if (header_end == std::string::npos)
    {
        header_end = buffer_.find("\n\n");
        if (header_end == std::string::npos)
        {
            return false;// Need more data
        }
        header_end += 2;
    }
    else
    {
        header_end += 4;
    }

    // Parse headers
    std::string header_data = buffer_.substr(0, header_end - 4);
    buffer_.erase(0, header_end);

    // Create new field
    current_field_ = std::make_unique<MultipartField>();

    // Parse headers (simplified version)
    std::istringstream stream(header_data);
    std::string line;

    while (std::getline(stream, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos)
        {
            std::string name = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);

            // Trim whitespace
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            current_field_->add_header(name, value);

            // Parse Content-Disposition
            if (name == "Content-Disposition")
            {
                // Extract name and filename (simplified)
                size_t name_pos = value.find("name=\"");
                if (name_pos != std::string::npos)
                {
                    name_pos += 6;
                    size_t name_end = value.find("\"", name_pos);
                    if (name_end != std::string::npos)
                    {
                        current_field_->set_name(value.substr(name_pos, name_end - name_pos));
                        current_field_name_ = current_field_->name();
                    }
                }

                size_t filename_pos = value.find("filename=\"");
                if (filename_pos != std::string::npos)
                {
                    filename_pos += 10;
                    size_t filename_end = value.find("\"", filename_pos);
                    if (filename_end != std::string::npos)
                    {
                        current_field_->set_filename(value.substr(filename_pos, filename_end - filename_pos));
                    }
                }
            }
        }
    }

    current_field_size_ = 0;
    state_ = ParseState::CONTENT;
    return true;
}

bool MultipartStreamParser::Impl::process_content()
{
    // Look for next boundary
    size_t boundary_pos = buffer_.find(full_boundary_);
    if (boundary_pos == std::string::npos)
    {
        // Check if we have enough data to contain a boundary
        if (buffer_.length() > full_boundary_.length() + 100)
        {
            // Process what we have
            write_content_data(buffer_.data(), buffer_.length() - full_boundary_.length() - 10);
            buffer_.clear();
        }
        return false;// Need more data
    }

    // Process content up to boundary
    if (boundary_pos > 0)
    {
        // Remove trailing CRLF
        size_t content_end = boundary_pos;
        while (content_end > 0 && (buffer_[content_end - 1] == '\n' || buffer_[content_end - 1] == '\r'))
        {
            content_end--;
        }

        if (content_end > 0)
        {
            write_content_data(buffer_.data(), content_end);
        }
    }

    // Remove processed data
    buffer_.erase(0, boundary_pos);

    // Finalize current field
    finalize_current_field();

    state_ = ParseState::BOUNDARY;
    return true;
}

void MultipartStreamParser::Impl::finalize_current_field()
{
    if (current_field_)
    {
        result_.push_back(std::move(current_field_));
        current_field_.reset();
    }

    if (progress_callback_)
    {
        progress_callback_(bytes_processed_, 0, current_field_name_);
    }
}

void MultipartStreamParser::Impl::handle_error(const std::string &error)
{
    state_ = ParseState::error;
    if (error_callback_)
    {
        error_callback_(error, bytes_processed_);
    }
}

void MultipartStreamParser::Impl::write_content_data(const char *data, std::size_t size)
{
    if (!current_field_)
    {
        return;
    }

    current_field_size_ += size;

    // Check size limits
    if (current_field_->is_file())
    {
        if (current_field_size_ > config_.max_file_size)
        {
            handle_error("File size exceeds maximum allowed size");
            return;
        }
    }
    else
    {
        if (current_field_size_ > config_.max_field_size)
        {
            handle_error("Field size exceeds maximum allowed size");
            return;
        }
    }

    // For now, just accumulate in memory
    // In a real implementation, you'd write to temp file for large content
    if (current_field_->is_file() && current_field_size_ > config_.memory_threshold)
    {
        switch_to_temp_file();
    }

    // This is a simplified implementation
    // Real implementation would handle temp files properly
}

void MultipartStreamParser::Impl::switch_to_temp_file()
{
    // Simplified implementation
    // Real implementation would create temp file and write to it
}

std::string MultipartStreamParser::Impl::generate_temp_filename() const
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);

    std::string filename = config_.temp_directory + "/foxhttp_stream_";

    for (int i = 0; i < 16; ++i)
    {
        filename += "0123456789abcdef"[dis(gen)];
    }

    return filename;
}

void MultipartStreamParser::Impl::cleanup_temp_files()
{
    for (auto &field: result_)
    {
        if (field)
        {
            field->cleanup_temp_file();
        }
    }
}

}// namespace foxhttp
