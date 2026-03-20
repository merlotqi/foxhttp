#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/bind/bind.hpp>
#include <foxhttp/server/session_base.hpp>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

namespace foxhttp {

class middleware_chain;
class session : public session_base, public std::enable_shared_from_this<session> {
 public:
  explicit session(tcp::socket socket, std::shared_ptr<middleware_chain> global_chain);
  void start();

 private:
  void read_request();
  void handle_read(boost::beast::error_code ec, size_t bytes_transferred);
  void process_request();
  void write_response();

  void on_timeout_idle() override;
  void on_timeout_header() override;
  void on_timeout_body() override;

 private:
  tcp::socket socket_;
  boost::beast::flat_buffer buffer_;
  http::request<http::string_body> req_;
  http::response<http::string_body> res_;
  std::shared_ptr<middleware_chain> global_chain_;
  std::size_t requests_served_{0};
};

}  // namespace foxhttp
