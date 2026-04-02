#ifdef USING_TLS

#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>
#include <chrono>
#include <foxhttp/foxhttp.hpp>
#include <thread>

TEST(SSLHandshakeTimeout, SSLServerStopMethodExists) {
  foxhttp::server::IoContextPool pool(1);
  boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tlsv12);

  foxhttp::server::SslServer server(pool, 0, ssl_ctx);

  // Verify stop method can be called without throwing
  EXPECT_NO_THROW(server.stop());
}

TEST(SSLHandshakeTimeout, SSLServerMultipleStopCallsAreSafe) {
  foxhttp::server::IoContextPool pool(1);
  boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tlsv12);

  foxhttp::server::SslServer server(pool, 0, ssl_ctx);

  // Multiple stop calls should be safe
  EXPECT_NO_THROW(server.stop());
  EXPECT_NO_THROW(server.stop());
  EXPECT_NO_THROW(server.stop());
}

TEST(SSLHandshakeTimeout, SSLServerDestructionIsSafe) {
  foxhttp::server::IoContextPool pool(1);
  boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tlsv12);

  {
    foxhttp::server::SslServer server(pool, 0, ssl_ctx);
    // Server goes out of scope and should be destroyed safely
  }

  // No crash or exception should occur
  SUCCEED();
}

TEST(SSLHandshakeTimeout, SSLServerStopDoesNotThrowWithActiveConnections) {
  foxhttp::server::IoContextPool pool(1);
  boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tlsv12);

  foxhttp::server::SslServer server(pool, 0, ssl_ctx);

  // Even with potential active connections, stop should not throw
  std::thread runner([&pool] { pool.run_blocking(); });

  // Give the pool some time to start
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Stop the server
  EXPECT_NO_THROW(server.stop());

  // Stop the pool
  pool.stop();

  if (runner.joinable()) {
    runner.join();
  }
}

TEST(SSLHandshakeTimeout, SSLContextInitialization) {
  // Test that SSL context can be initialized properly
  EXPECT_NO_THROW({
    boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tlsv12);
    foxhttp::server::IoContextPool pool(1);
    foxhttp::server::SslServer server(pool, 0, ssl_ctx);
  });
}

#endif
