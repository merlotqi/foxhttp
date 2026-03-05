#pragma once

#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <map>
#include <string>

namespace asio = boost::asio;
namespace http = boost::beast::http;

namespace foxhttp {

class response {
 public:
  explicit response(http::response<http::string_body> &&res);

  bool is_ok() const;
  bool is_successful() const;
  int status_code() const;
  std::string status_text() const;

  const std::string &body() const;
  std::string body();

  bool has_header(const std::string &name) const;
  std::string get_header(const std::string &name) const;
  std::map<std::string, std::string> headers() const;

  const http::response<http::string_body> &native() const;
  http::response<http::string_body> &native();

 private:
  http::response<http::string_body> response_;
};

class request {
 public:
  explicit request(const std::string &url);

  const std::string &url() const;
  const std::string &protocol() const;
  const std::string &host() const;
  const std::string &port() const;
  const std::string &target() const;

  request &method(const std::string &m);
  std::string method() const;

  request &header(const std::string &name, const std::string &value);
  request &headers(const std::map<std::string, std::string> &h);

  request &body(const std::string &b);
  const std::string &body() const;

  request &verify_ssl(bool verify);
  bool verify_ssl() const;

  request &timeout(int seconds);
  int timeout() const;

  const http::request<http::string_body> &native() const;
  http::request<http::string_body> &native();

 private:
  std::string url_;
  std::string protocol_;
  std::string host_;
  std::string port_;
  std::string target_;
  http::request<http::string_body> request_;
  bool verify_ssl_ = true;
  int timeout_ = 30;

  static void _parse_url(const std::string &url, std::string &protocol, std::string &host, std::string &port,
                         std::string &target);
};

}  // namespace foxhttp
