#include <benchmark/benchmark.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/http.hpp>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <foxhttp/foxhttp.hpp>
#include <memory>
#include <string>
#include <thread>

#if !defined(_WIN32)
namespace {
struct IgnoreSigpipeForBenchmark {
  IgnoreSigpipeForBenchmark() { std::signal(SIGPIPE, SIG_IGN); }
} g_ignore_sigpipe;
}  // namespace
#endif

#if defined(_WIN32)
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace http = boost::beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

namespace {

std::shared_ptr<foxhttp::middleware::MiddlewareChain> make_ok_chain(asio::io_context &ioc) {
  auto c = std::make_shared<foxhttp::middleware::MiddlewareChain>(ioc);
  c->use(std::make_shared<foxhttp::middleware::FunctionalMiddleware>(
      "bench_ok", [](foxhttp::server::RequestContext &, http::response<http::string_body> &res, std::function<void()> next) {
        res.result(http::status::ok);
        res.set(http::field::content_type, "text/plain");
        res.body() = "ok";
        next();
      }));
  return c;
}

asio::awaitable<void> accept_forever(tcp::acceptor acc, std::shared_ptr<foxhttp::middleware::MiddlewareChain> chain) {
  for (;;) {
    tcp::socket sock = co_await acc.async_accept(asio::use_awaitable);
    std::make_shared<foxhttp::server::Session>(std::move(sock), chain)->start();
  }
}

class LoopbackServer {
 public:
  explicit LoopbackServer() : ioc_(), chain_(make_ok_chain(ioc_)) {
    tcp::acceptor acc(ioc_, tcp::endpoint(asio::ip::address_v4::loopback(), 0));
    port_ = acc.local_endpoint().port();
    asio::co_spawn(ioc_, accept_forever(std::move(acc), chain_), asio::detached);
    thread_ = std::thread([this] { ioc_.run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  LoopbackServer(const LoopbackServer &) = delete;
  LoopbackServer &operator=(const LoopbackServer &) = delete;

  ~LoopbackServer() {
    ioc_.stop();
    if (thread_.joinable()) thread_.join();
  }

  std::uint16_t port() const { return port_; }

 private:
  asio::io_context ioc_;
  std::shared_ptr<foxhttp::middleware::MiddlewareChain> chain_;
  std::thread thread_;
  std::uint16_t port_{};
};

#if !defined(_WIN32)

int dial_loopback(std::uint16_t port) {
  int fd = static_cast<int>(::socket(AF_INET, SOCK_STREAM, 0));
  if (fd < 0) return -1;
  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (::connect(fd, reinterpret_cast<sockaddr *>(&a), sizeof(a)) != 0) {
    ::close(fd);
    return -1;
  }
  return fd;
}

void send_all(int fd, const char *p, std::size_t n) {
#if defined(MSG_NOSIGNAL)
  const int send_flags = MSG_NOSIGNAL;
#else
  const int send_flags = 0;
#endif
  while (n > 0) {
    const ssize_t w = ::send(fd, p, n, send_flags);
    if (w <= 0) return;
    p += static_cast<std::size_t>(w);
    n -= static_cast<std::size_t>(w);
  }
}

// Read until double CRLF (end of headers) plus small body; enough for our "ok" reply.
void drain_http_response(int fd) {
  char buf[2048];
  std::size_t total = 0;
  while (total < sizeof(buf)) {
    const ssize_t n = ::recv(fd, buf + total, sizeof(buf) - total, 0);
    if (n <= 0) break;
    total += static_cast<std::size_t>(n);
    for (std::size_t i = 3; i < total; ++i) {
      if (buf[i - 3] == '\r' && buf[i - 2] == '\n' && buf[i - 1] == '\r' && buf[i] == '\n') {
        return;
      }
    }
  }
}

static constexpr std::string_view k_http10_get = "GET / HTTP/1.0\r\n\r\n";
static constexpr std::string_view k_http11_keepalive =
    "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n";

#endif

}  // namespace

/// Measures `await_middleware_chain_async` + a one-layer sync chain (no TCP).
static void BM_AwaitMiddlewareBridge(benchmark::State &state) {
  const int inner = static_cast<int>(state.range(0));
  for (auto _ : state) {
    asio::io_context ioc;
    auto chain = make_ok_chain(ioc);
    http::request<http::string_body> req;
    req.method(http::verb::get);
    req.target("/");
    req.version(11);

    asio::co_spawn(
        ioc,
        [&]() -> asio::awaitable<void> {
          for (int i = 0; i < inner; ++i) {
            foxhttp::server::RequestContext ctx(req);
            http::response<http::string_body> res{http::status::internal_server_error, 11};
            co_await foxhttp::detail::await_middleware_chain_async(ioc.get_executor(), chain, ctx, res);
          }
        },
        asio::detached);
    ioc.run();
  }
  state.SetItemsProcessed(state.iterations() * inner);
}
BENCHMARK(BM_AwaitMiddlewareBridge)->Arg(32)->Arg(256)->Arg(2048);

#if !defined(_WIN32)

/// One new TCP Connection per request (HTTP/1.0); exercises full coroutine session path + accept loop.
static void BM_SessionCoroutine_HTTP10_cold(benchmark::State &state) {
  const int reqs = static_cast<int>(state.range(0));
  LoopbackServer srv;
  for (auto _ : state) {
    for (int i = 0; i < reqs; ++i) {
      int fd = dial_loopback(srv.port());
      if (fd < 0) {
        state.SkipWithError("connect failed");
        return;
      }
      send_all(fd, k_http10_get.data(), k_http10_get.size());
      drain_http_response(fd);
      ::close(fd);
    }
  }
  state.SetItemsProcessed(state.iterations() * reqs);
}
BENCHMARK(BM_SessionCoroutine_HTTP10_cold)->Arg(8)->Arg(32);

/// Single Connection, many HTTP/1.1 keep-alive requests; coroutine session loop reuse.
static void BM_SessionCoroutine_HTTP11_keepalive(benchmark::State &state) {
  const int reqs = static_cast<int>(state.range(0));
  LoopbackServer srv;
  for (auto _ : state) {
    int fd = dial_loopback(srv.port());
    if (fd < 0) {
      state.SkipWithError("connect failed");
      return;
    }
    for (int i = 0; i < reqs; ++i) {
      send_all(fd, k_http11_keepalive.data(), k_http11_keepalive.size());
      drain_http_response(fd);
    }
    ::close(fd);
  }
  state.SetItemsProcessed(state.iterations() * reqs);
}
BENCHMARK(BM_SessionCoroutine_HTTP11_keepalive)->Arg(64)->Arg(512)->Arg(4096);

#else

static void BM_SessionCoroutine_skipped_on_win32(benchmark::State &state) {
  state.SkipWithError("loopback benchmarks use POSIX sockets in this build");
}
BENCHMARK(BM_SessionCoroutine_skipped_on_win32);

#endif
