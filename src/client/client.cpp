#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/client/client.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = asio::ssl;
using tcp = asio::ip::tcp;

namespace foxhttp {
namespace details {
class client {
 private:
  static void _parse_url(const std::string &url, std::string &protocol, std::string &host, std::string &port,
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

  response _execute_request(request &req) {
    const std::string &protocol = req.protocol();
    const std::string &host = req.host();
    const std::string &port = req.port();
    const std::string &target = req.target();
    return protocol == "https" ? _execute_https_request(req, host, port, target)
                               : _execute_http_request(req, host, port, target);
  }

  response _execute_http_request(request &req, const std::string &host, const std::string &port,
                                const std::string &target) {
    asio::io_context ioc;
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);

    auto const results = resolver.resolve(host, port);
    stream.connect(results);

    auto http_req = req.native();

    http::write(stream, http_req);

    beast::flat_buffer buffer;
    http::response<http::string_body> http_res;
    http::read(stream, buffer, http_res);

    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);

    return response{std::move(http_res)};
  }

  response _execute_https_request(request &req, const std::string &host, const std::string &port,
                                 const std::string &target) {
    asio::io_context ioc;
    ssl::context ctx(ssl::context::tlsv12_client);
    if (!req.verify_ssl())
      ctx.set_verify_mode(ssl::verify_none);
    else
      ctx.set_default_verify_paths();

    tcp::resolver resolver(ioc);
    ssl::stream<tcp::socket> stream(ioc, ctx);

    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
      beast::error_code ec{static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
      throw beast::system_error{ec};
    }

    auto const results = resolver.resolve(host, port);
    asio::connect(stream.next_layer(), results.begin(), results.end());
    stream.handshake(ssl::stream_base::client);

    auto http_req = req.native();

    http::write(stream, http_req);

    beast::flat_buffer buffer;
    http::response<http::string_body> http_res;
    http::read(stream, buffer, http_res);

    beast::error_code ec;
    stream.shutdown(ec);

    return response{std::move(http_res)};
  }

  response _send_request_sync(request &req) { return _execute_request(req); }

 public:
  promise<response> fetch(const std::string &url) { return fetch(request{url}); }
  promise<response> fetch(request req) {
    promise<response> pmse;
    std::thread([this, req = std::move(req), pmse]() mutable {
      try {
        response res = this->_send_request_sync(const_cast<request &>(req));
        pmse.resolve(res);
      } catch (const std::exception &e) {
        pmse.reject(e);
      }
    }).detach();
    return pmse;
  }
};

}  // namespace details

static details::client &global_client_instance() {
  static details::client cli;
  return cli;
}

promise<response> fetch(const std::string &url) { return global_client_instance().fetch(url); }

promise<response> fetch(request req) { return global_client_instance().fetch(std::move(req)); }

}  // namespace foxhttp
