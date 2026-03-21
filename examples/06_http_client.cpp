#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/client/http_client.hpp>

#include <future>
#include <iostream>
#include <string>
#include <thread>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;

static void one_shot_server(std::promise<unsigned short> port_out) {
  try {
    asio::io_context ioc;
    tcp::acceptor acc(ioc, tcp::endpoint{asio::ip::make_address_v4("127.0.0.1"), 0});
    port_out.set_value(acc.local_endpoint().port());
    tcp::socket sock(ioc);
    acc.accept(sock);
    beast::flat_buffer buf;
    http::request<http::string_body> req;
    beast::error_code ec;
    http::read(sock, buf, req, ec);
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "text/plain");
    res.body() = "Hello from 06_http_client stub\n";
    res.prepare_payload();
    http::write(sock, res, ec);
  } catch (...) {
    try {
      port_out.set_exception(std::current_exception());
    } catch (...) {
    }
  }
}

int main() {
  std::promise<unsigned short> p;
  auto f = p.get_future();
  std::thread srv(one_shot_server, std::move(p));
  const unsigned short port = f.get();
  const std::string base = "http://127.0.0.1:" + std::to_string(port);

  asio::io_context ioc;
  foxhttp::client::http_client client(ioc.get_executor(), base);

  // --- Promise-style chain (callbacks run on the client's executor) ---
  client.post("/demo")
      .body_json({{"msg", "chain"}})
      .catch_error([&](const boost::system::error_code &ec) {
        std::cerr << "chain error: " << ec.message() << '\n';
        ioc.stop();
      })
      .then([&](const http::response<http::string_body> &res) {
        std::cout << "then: " << static_cast<unsigned>(res.result()) << ' ' << res.body();
      });

  ioc.restart();
  ioc.run();

  // Start server again for the coroutine leg (one accept per connection).
  std::promise<unsigned short> p2;
  auto f2 = p2.get_future();
  std::thread srv2(one_shot_server, std::move(p2));
  const unsigned short port2 = f2.get();
  foxhttp::client::http_client client2(ioc.get_executor(),
                                       "http://127.0.0.1:" + std::to_string(port2));

  asio::co_spawn(
      ioc,
      [&]() -> asio::awaitable<void> {
        try {
          auto res = co_await client2.get("/await").as_awaitable();
          std::cout << "await: " << static_cast<unsigned>(res.result()) << ' ' << res.body();
        } catch (const std::exception &ex) {
          std::cerr << "await error: " << ex.what() << '\n';
        }
        co_return;
      },
      asio::detached);

  ioc.restart();
  ioc.run();

  srv.join();
  srv2.join();
  return 0;
}
