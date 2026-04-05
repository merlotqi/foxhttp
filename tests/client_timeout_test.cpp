#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>
#include <foxhttp/foxhttp.hpp>

using namespace foxhttp::client;

TEST(ClientOptionsTest, DefaultValues) {
  ClientOptions opts;
  EXPECT_EQ(opts.connection_timeout.count(), 30000);
  EXPECT_EQ(opts.request_timeout.count(), 30000);
  EXPECT_EQ(opts.handshake_timeout.count(), 30000);
  EXPECT_EQ(opts.max_response_size, 0u);
  EXPECT_FALSE(opts.follow_redirects);
  EXPECT_EQ(opts.max_redirects, 5u);
}

TEST(ClientOptionsTest, FluentSetters) {
  ClientOptions opts;
  opts.set_connection_timeout(std::chrono::milliseconds(5000))
      .set_request_timeout(std::chrono::milliseconds(10000))
      .set_handshake_timeout(std::chrono::milliseconds(15000))
      .set_max_response_size(1024 * 1024)
      .set_follow_redirects(true, 10)
      .set_user_agent("TestAgent/1.0");

  EXPECT_EQ(opts.connection_timeout.count(), 5000);
  EXPECT_EQ(opts.request_timeout.count(), 10000);
  EXPECT_EQ(opts.handshake_timeout.count(), 15000);
  EXPECT_EQ(opts.max_response_size, 1024u * 1024);
  EXPECT_TRUE(opts.follow_redirects);
  EXPECT_EQ(opts.max_redirects, 10u);
  EXPECT_EQ(opts.user_agent, "TestAgent/1.0");
}

TEST(RequestTimeoutOptionsTest, DefaultValues) {
  RequestTimeoutOptions opts;
  EXPECT_EQ(opts.connection_timeout.count(), 0);
  EXPECT_EQ(opts.request_timeout.count(), 0);
  EXPECT_EQ(opts.handshake_timeout.count(), 0);
}

TEST(RequestTimeoutOptionsTest, FluentSetters) {
  RequestTimeoutOptions opts;
  opts.set_connection_timeout(std::chrono::milliseconds(2000))
      .set_request_timeout(std::chrono::milliseconds(3000))
      .set_handshake_timeout(std::chrono::milliseconds(4000));

  EXPECT_EQ(opts.connection_timeout.count(), 2000);
  EXPECT_EQ(opts.request_timeout.count(), 3000);
  EXPECT_EQ(opts.handshake_timeout.count(), 4000);
}

TEST(HttpClientTest, SetOptions) {
  boost::asio::io_context ioc;
  HttpClient client(ioc.get_executor(), "http://localhost:8080");

  ClientOptions opts;
  opts.set_connection_timeout(std::chrono::milliseconds(5000)).set_request_timeout(std::chrono::milliseconds(10000));

  client.set_options(opts);

  EXPECT_EQ(client.get_options().connection_timeout.count(), 5000);
  EXPECT_EQ(client.get_options().request_timeout.count(), 10000);
}

TEST(RequestBuilderTest, TimeoutMethods) {
  boost::asio::io_context ioc;
  HttpClient client(ioc.get_executor(), "http://localhost:8080");

  auto builder = client.get("/test")
                     .connection_timeout(std::chrono::milliseconds(1000))
                     .request_timeout(std::chrono::milliseconds(2000));

  // Verify builder was created successfully (no exceptions)
  SUCCEED();
}

TEST(RequestBuilderTest, TimeoutWithOptions) {
  boost::asio::io_context ioc;
  HttpClient client(ioc.get_executor(), "http://localhost:8080");

  RequestTimeoutOptions timeout_opts;
  timeout_opts.set_connection_timeout(std::chrono::milliseconds(1500))
      .set_request_timeout(std::chrono::milliseconds(2500));

  auto builder = client.get("/test").timeout(timeout_opts);

  // Verify builder was created successfully (no exceptions)
  SUCCEED();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
