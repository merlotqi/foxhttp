#include <foxhttp/client/types.hpp>

namespace foxhttp {

/* -------------------------------- response -------------------------------- */

response::response(http::response<http::string_body> &&res) : response_(std::move(res)) {}

bool response::is_ok() const { return response_.result() == http::status::ok; }

bool response::is_successful() const { return response_.result_int() >= 200 && response_.result_int() < 300; }

int response::status_code() const { return response_.result_int(); }

std::string response::status_text() const { return response_.reason(); }

const std::string &response::body() const { return response_.body(); }

std::string response::body() { return response_.body(); }

bool response::has_header(const std::string &name) const { return response_.find(name) != response_.end(); }

std::string response::get_header(const std::string &name) const {
  auto it = response_.find(name);
  return it != response_.end() ? std::string(it->value()) : "";
}

std::map<std::string, std::string> response::headers() const {
  std::map<std::string, std::string> result;
  for (const auto &field : response_) {
    result[std::string(field.name_string())] = std::string(field.value());
  }
  return result;
}

const http::response<http::string_body> &response::native() const { return response_; }

http::response<http::string_body> &response::native() { return response_; }

/* --------------------------------- Request -------------------------------- */

static void parse_url_impl(const std::string &url, std::string &protocol, std::string &host, std::string &port,
                           std::string &target) {
  size_t protocol_end = url.find("://");
  if (protocol_end == std::string::npos) return;
  protocol = url.substr(0, protocol_end);
  size_t host_start = protocol_end + 3;
  size_t host_end = url.find('/', host_start);
  if (host_end == std::string::npos) {
    host = url.substr(host_start);
    target = "/";
  } else {
    host = url.substr(host_start, host_end - host_start);
    target = url.substr(host_end);
  }
  size_t port_start = host.find(':');
  if (port_start != std::string::npos) {
    port = host.substr(port_start + 1);
    host = host.substr(0, port_start);
  } else {
    port = (protocol == "https") ? "443" : "80";
  }
}

request::request(const std::string &url) : url_(url) {
  parse_url(url, protocol_, host_, port_, target_);
  request_.target(target_);
  request_.method(http::verb::get);
  request_.version(11);
  request_.set(http::field::host, host_);
  request_.set(http::field::user_agent, "foxhttp/1.0");
}

const std::string &request::url() const { return url_; }
const std::string &request::protocol() const { return protocol_; }
const std::string &request::host() const { return host_; }
const std::string &request::port() const { return port_; }
const std::string &request::target() const { return target_; }

request &request::method(const std::string &m) {
  request_.method(http::string_to_verb(m));
  return *this;
}

std::string request::method() const { return std::string(request_.method_string()); }

request &request::header(const std::string &name, const std::string &value) {
  request_.set(name, value);
  return *this;
}

request &request::headers(const std::map<std::string, std::string> &h) {
  for (const auto &pair : h) {
    request_.set(pair.first, pair.second);
  }
  return *this;
}

request &request::body(const std::string &b) {
  request_.body() = b;
  request_.prepare_payload();
  return *this;
}

const std::string &request::body() const { return request_.body(); }

request &request::verify_ssl(bool verify) {
  verify_ssl_ = verify;
  return *this;
}

bool request::verify_ssl() const { return verify_ssl_; }

request &request::timeout(int seconds) {
  timeout_ = seconds;
  return *this;
}

int request::timeout() const { return timeout_; }

const http::request<http::string_body> &request::native() const { return request_; }

http::request<http::string_body> &request::native() { return request_; }

void request::parse_url(const std::string &url, std::string &protocol, std::string &host, std::string &port,
                        std::string &target) {
  parse_url_impl(url, protocol, host, port, target);
}

}  // namespace foxhttp
