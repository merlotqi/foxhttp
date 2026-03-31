#include <gtest/gtest.h>

#include <boost/beast/http.hpp>
#include <chrono>
#include <filesystem>
#include <foxhttp/foxhttp.hpp>
#include <fstream>
#include <string>

namespace http = boost::beast::http;

namespace {

class TempDir {
 public:
  explicit TempDir(const std::string &name) {
    const auto id = std::chrono::steady_clock::now().time_since_epoch().count();
    root_ = std::filesystem::temp_directory_path() / ("foxhttp_" + name + "_" + std::to_string(id));
    std::filesystem::create_directories(root_);
  }
  ~TempDir() {
    std::error_code ec;
    std::filesystem::remove_all(root_, ec);
  }
  const std::filesystem::path &path() const { return root_; }

 private:
  std::filesystem::path root_;
};

}  // namespace

TEST(StaticMiddleware, GetServesFileAndStopsChain) {
  TempDir td("static_get");
  {
    std::ofstream f(td.path() / "a.txt");
    f << "hello";
  }

  foxhttp::static_middleware mw("/assets", td.path());
  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target("/assets/a.txt");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;
  bool next_called = false;
  mw(ctx, res, [&] { next_called = true; });

  EXPECT_FALSE(next_called);
  EXPECT_EQ(res.result(), http::status::ok);
  EXPECT_EQ(res.body(), "hello");
  EXPECT_EQ(res[http::field::content_type], "text/plain; charset=utf-8");
}

TEST(StaticMiddleware, HeadHasLengthAndEmptyBody) {
  TempDir td("static_head");
  {
    std::ofstream f(td.path() / "b.bin");
    f << "12345";
  }

  foxhttp::static_middleware mw("/s", td.path());
  http::request<http::string_body> req;
  req.method(http::verb::head);
  req.target("/s/b.bin");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;
  mw(ctx, res, [] {});

  EXPECT_EQ(res.result(), http::status::ok);
  EXPECT_TRUE(res.body().empty());
  EXPECT_EQ(res[http::field::content_type], "application/octet-stream");
}

TEST(StaticMiddleware, PostDelegatesToNext) {
  TempDir td("static_post");
  foxhttp::static_middleware mw("/assets", td.path());
  http::request<http::string_body> req;
  req.method(http::verb::post);
  req.target("/assets/x");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;
  bool next_called = false;
  mw(ctx, res, [&] { next_called = true; });
  EXPECT_TRUE(next_called);
}

TEST(StaticMiddleware, PrefixMismatchDelegatesToNext) {
  TempDir td("static_nomatch");
  foxhttp::static_middleware mw("/assets", td.path());
  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target("/other/file");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;
  bool next_called = false;
  mw(ctx, res, [&] { next_called = true; });
  EXPECT_TRUE(next_called);
}

TEST(StaticMiddleware, PayloadTooLarge) {
  TempDir td("static_big");
  {
    std::ofstream f(td.path() / "big.bin");
    std::string chunk(5000, 'x');
    for (int i = 0; i < 20; ++i) f << chunk;
  }

  foxhttp::static_middleware mw("/x", td.path());
  mw.set_max_file_size(1000);
  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target("/x/big.bin");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;
  mw(ctx, res, [] { FAIL() << "next should not run"; });
  EXPECT_EQ(res.result(), http::status::payload_too_large);
}

TEST(StaticMiddleware, ServesIndexHtmlForDirectory) {
  TempDir td("static_index");
  {
    std::ofstream f(td.path() / "index.html");
    f << "<html/>";
  }

  foxhttp::static_middleware mw("/pub", td.path());
  http::request<http::string_body> req;
  req.method(http::verb::get);
  req.target("/pub");
  foxhttp::request_context ctx(req);
  http::response<http::string_body> res;
  mw(ctx, res, [] { FAIL() << "next"; });
  EXPECT_EQ(res.result(), http::status::ok);
  EXPECT_EQ(res.body(), "<html/>");
}
