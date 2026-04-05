#include <boost/beast/http.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <foxhttp/middleware/advanced/compression_middleware.hpp>
#include <sstream>
#include <string>
#include <vector>
#ifdef USING_BROTLI
#include <brotli/encode.h>
#endif

namespace foxhttp::middleware::advanced {
CompressionMiddleware::CompressionMiddleware(size_t threshold, bool enable_gzip, bool enable_deflate, bool enable_br)
    : threshold_(threshold) {
  if (enable_gzip) supported_types_.push_back(CompressionType::gzip);
  if (enable_deflate) supported_types_.push_back(CompressionType::deflate);
  if (enable_br) supported_types_.push_back(CompressionType::br);
}

std::string CompressionMiddleware::name() const { return "CompressionMiddleware"; }

void CompressionMiddleware::operator()(RequestContext &ctx, http::response<http::string_body> &res,
                                        std::function<void()> next) {
  next();

  if (should_compress(ctx, res)) {
    compress_response(ctx, res);
  }
}

void CompressionMiddleware::operator()(RequestContext &ctx, http::response<http::string_body> &res,
                                        std::function<void()> next, async_middleware_callback callback) {
  next();

  if (should_compress(ctx, res)) {
    compress_response(ctx, res);
  }

  callback(MiddlewareResult::Continue, "");
}

bool CompressionMiddleware::should_compress(const RequestContext &ctx, const http::response<http::string_body> &res) {
  // Don't compress if already compressed
  if (res.find("Content-Encoding") != res.end()) {
    return false;
  }

  // Don't compress if body is too small
  if (res.body().size() < threshold_) {
    return false;
  }

  // Check if client accepts any of our supported compression types
  auto accept_encoding = ctx.header("Accept-Encoding");
  if (accept_encoding.empty()) {
    return false;
  }

  for (const auto &type : supported_types_) {
    if (accept_encoding.find(get_encoding_name(type)) != std::string::npos) {
      return true;
    }
  }

  return false;
}

void CompressionMiddleware::compress_response(const RequestContext &ctx, http::response<http::string_body> &res) {
  auto accept_encoding = ctx.header("Accept-Encoding");
  CompressionType best_type = CompressionType::none;
  int best_priority = -1;

  // Find the best supported compression type that the client accepts
  for (const auto &type : supported_types_) {
    std::string encoding = get_encoding_name(type);
    size_t pos = accept_encoding.find(encoding);
    if (pos != std::string::npos) {
      int priority = 1;  // Default priority
      // Check if there's a quality value (q-value)
      size_t q_pos = accept_encoding.find("q=", pos);
      if (q_pos != std::string::npos) {
        size_t end = accept_encoding.find(";", q_pos);
        std::string q_value = accept_encoding.substr(q_pos + 2, end - q_pos - 2);
        try {
          priority = static_cast<int>(std::stof(q_value) * 1000);
        } catch (...) {
          // Use default priority if parsing fails
        }
      }
      if (priority > best_priority) {
        best_priority = priority;
        best_type = type;
      }
    }
  }

  if (best_type != CompressionType::none) {
    std::string compressed;
    switch (best_type) {
      case CompressionType::gzip:
        compressed = compress_gzip(res.body());
        break;
      case CompressionType::deflate:
        compressed = compress_deflate(res.body());
        break;
      case CompressionType::br:
        compressed = compress_brotli(res.body());
        break;
      default:
        return;
    }

    res.set("Content-Encoding", get_encoding_name(best_type));
    res.body() = compressed;
    // Update Content-Length after compression
    res.set("Content-Length", std::to_string(compressed.size()));
  }
}

std::string CompressionMiddleware::get_encoding_name(CompressionType type) {
  switch (type) {
    case CompressionType::gzip:
      return "gzip";
    case CompressionType::deflate:
      return "deflate";
    case CompressionType::br:
      return "br";
    default:
      return "";
  }
}

std::string CompressionMiddleware::compress_gzip(const std::string &input) {
  namespace bio = boost::iostreams;
  std::stringstream compressed;
  std::stringstream origin(input);

  bio::filtering_streambuf<bio::input> out;
  out.push(bio::gzip_compressor(bio::gzip_params(bio::gzip::best_compression)));
  out.push(origin);
  bio::copy(out, compressed);

  return compressed.str();
}

std::string CompressionMiddleware::compress_deflate(const std::string &input) {
  namespace bio = boost::iostreams;
  std::stringstream compressed;
  std::stringstream origin(input);

  try {
    bio::filtering_streambuf<bio::input> out;
    bio::zlib_params params(bio::zlib::best_compression);
    params.window_bits = -15;
    out.push(bio::zlib_compressor(params));
    out.push(origin);

    std::istream input_stream(&out);
    compressed << input_stream.rdbuf();
  } catch (const std::exception &e) {
    // If compression fails, return original input
    return input;
  }

  return compressed.str();
}

std::string CompressionMiddleware::compress_brotli(const std::string &input) {
#ifdef USING_BROTLI
  // Allocate output buffer (typically 1.5x input size for worst case)
  std::vector<uint8_t> output(BrotliEncoderMaxCompressedSize(input.size()));

  size_t encoded_size = output.size();
  BROTLI_BOOL result = BrotliEncoderCompress(BROTLI_DEFAULT_QUALITY,  // Compression quality (0-11)
                                             BROTLI_DEFAULT_WINDOW,   // Window size (10-24)
                                             BROTLI_DEFAULT_MODE,     // Compression mode (0=generic, 1=text, 2=font)
                                             input.size(),            // Input size
                                             reinterpret_cast<const uint8_t *>(input.data()),  // Input data
                                             &encoded_size,                                    // Output size
                                             output.data()                                     // Output data
  );

  if (!result) {
    // If compression fails, fallback to gzip
    return compress_gzip(input);
  }

  // Resize the output vector to the actual compressed size
  output.resize(encoded_size);
  return std::string(output.begin(), output.end());
#else
  // Fallback to gzip if Brotli is not available
  return compress_gzip(input);
#endif
}

}  // namespace foxhttp::middleware::advanced
