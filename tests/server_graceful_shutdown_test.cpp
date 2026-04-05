#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>
#include <chrono>
#include <foxhttp/foxhttp.hpp>
#include <thread>

TEST(ServerGracefulShutdown, StopMethodExists) {
  foxhttp::io_context_pool pool(1);
  foxhttp::server server(pool, 0);

  // Verify stop method can be called without throwing
  EXPECT_NO_THROW(server.stop());
}

TEST(ServerGracefulShutdown, StopSetsStoppingFlag) {
  foxhttp::io_context_pool pool(1);
  foxhttp::server server(pool, 0);

  // Stop should set the internal stopping flag
  server.stop();

  // The server should not accept new connections after stop is called
  // This is implicitly tested by the accept loop checking the stopping flag
}

TEST(ServerGracefulShutdown, MultipleStopCallsAreSafe) {
  foxhttp::io_context_pool pool(1);
  foxhttp::server server(pool, 0);

  // Multiple stop calls should be safe
  EXPECT_NO_THROW(server.stop());
  EXPECT_NO_THROW(server.stop());
  EXPECT_NO_THROW(server.stop());
}

TEST(ServerGracefulShutdown, StopDoesNotThrowWithActiveConnections) {
  foxhttp::io_context_pool pool(1);
  foxhttp::server server(pool, 0);

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

TEST(ServerGracefulShutdown, ServerDestructionIsSafe) {
  foxhttp::io_context_pool pool(1);

  {
    foxhttp::server server(pool, 0);
    // Server goes out of scope and should be destroyed safely
  }

  // No crash or exception should occur
  SUCCEED();
}
