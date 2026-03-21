#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <foxhttp/client/http_client.hpp>

#include <chrono>
#include <future>
#include <string>
#include <thread>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;

namespace {

/// One-shot HTTP/1.1 server: reads one request, captures fields, responds with fixed body.
void run_loopback_server(std::promise<unsigned short> port_promise, std::string *captured_method,
                         std::string *captured_target, std::string *captured_body) {
  try {
    asio::io_context ioc;
    tcp::acceptor acc(ioc, tcp::endpoint{asio::ip::make_address_v4("127.0.0.1"), 0});
    port_promise.set_value(acc.local_endpoint().port());

    tcp::socket sock(ioc);
    acc.accept(sock);

    beast::flat_buffer buffer;
    http::request<http::string_body> req;
    beast::error_code ec;
    http::read(sock, buffer, req, ec);
    if (ec) {
      return;
    }
    if (captured_method) *captured_method = std::string(req.method_string());
    if (captured_target) *captured_target = std::string(req.target());
    if (captured_body) *captured_body = req.body();

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "foxhttp-test");
    res.set(http::field::content_type, "text/plain");
    res.body() = "ok-body";
    res.prepare_payload();
    http::write(sock, res, ec);
  } catch (...) {
    try {
      port_promise.set_exception(std::current_exception());
    } catch (...) {
    }
  }
}

}  // namespace

TEST(HttpClient, ThenChainPostsAndReadsBody) {
  std::promise<unsigned short> port_p;
  auto port_f = port_p.get_future();
  std::string method, target, body_in;
  std::thread srv(run_loopback_server, std::move(port_p), &method, &target, &body_in);
  const unsigned short port = port_f.get();

  asio::io_context ioc;
  foxhttp::client::http_client client(ioc.get_executor(),
                                      "http://127.0.0.1:" + std::to_string(port));

  bool done = false;
  client.post("/echo?q=1")
      .header("X-Test", "1")
      .body("hello-payload")
      .catch_error([&](const boost::system::error_code &ec) {
        FAIL() << "unexpected error: " << ec.message();
        ioc.stop();
      })
      .then([&](const http::response<http::string_body> &res) {
        EXPECT_EQ(res.result(), http::status::ok);
        EXPECT_EQ(res.body(), "ok-body");
        done = true;
        ioc.stop();
      });

  ioc.run();
  EXPECT_TRUE(done);
  srv.join();

  EXPECT_EQ(method, "POST");
  EXPECT_EQ(target, "/echo?q=1");
  EXPECT_EQ(body_in, "hello-payload");
}

TEST(HttpClient, CoAwaitAsAwaitable) {
  std::promise<unsigned short> port_p;
  auto port_f = port_p.get_future();
  std::thread srv(run_loopback_server, std::move(port_p), nullptr, nullptr, nullptr);
  const unsigned short port = port_f.get();

  asio::io_context ioc;
  foxhttp::client::http_client client(ioc.get_executor(),
                                      "http://127.0.0.1:" + std::to_string(port));

  asio::co_spawn(
      ioc,
      [&]() -> asio::awaitable<void> {
        auto res = co_await client.get("/status").as_awaitable();
        EXPECT_EQ(res.result(), http::status::ok);
        EXPECT_EQ(res.body(), "ok-body");
        ioc.stop();
        co_return;
      },
      asio::detached);

  ioc.run();
  srv.join();
}

TEST(HttpClient, CatchErrorOnConnectionRefused) {
  asio::io_context ioc;
  foxhttp::client::http_client client(ioc.get_executor(), "http://127.0.0.1:59999");

  bool got_error = false;
  bool got_success = false;
  client.get("/")
      .catch_error([&](const boost::system::error_code &ec) {
        EXPECT_TRUE(ec);
        got_error = true;
        ioc.stop();
      })
      .then([&](const http::response<http::string_body> &) {
        got_success = true;
        ioc.stop();
      });

  ioc.run();
  EXPECT_TRUE(got_error);
  EXPECT_FALSE(got_success);
}

TEST(HttpClient, BuilderCannotBeReusedAfterThen) {
  asio::io_context ioc;
  foxhttp::client::http_client client(ioc.get_executor(), "http://127.0.0.1:59999");

  auto builder = client.get("/");
  builder.catch_error([&](const boost::system::error_code &) { ioc.stop(); });
  builder.then([&](const http::response<http::string_body> &) {});
  EXPECT_THROW((void)builder.header("a", "b"), std::logic_error);
  ioc.run();
}

TEST(HttpClient, AsAwaitableCannotMixWithThen) {
  asio::io_context ioc;
  foxhttp::client::http_client client(ioc.get_executor(), "http://127.0.0.1:59999");

  auto builder = client.get("/");
  (void)builder.as_awaitable();
  EXPECT_THROW((void)builder.then([](const auto &) {}), std::logic_error);
}
