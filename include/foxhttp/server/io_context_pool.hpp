#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include <vector>

namespace foxhttp::server {

class IoContextPool {
 public:
  explicit IoContextPool(std::size_t pool_size = std::thread::hardware_concurrency());
  ~IoContextPool();

  /// Runs worker threads and blocks the calling thread until stop() is invoked elsewhere.
  void run_blocking();
  /// Same as run_blocking(); kept for existing call sites.
  void start();

  boost::asio::io_context &get_io_context();
  std::size_t size() const;

  void stop();

 private:
  std::vector<std::shared_ptr<boost::asio::io_context>> io_contexts_;
  std::vector<std::shared_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>> works_;
  std::vector<std::thread> threads_;
  std::atomic<std::size_t> next_io_context_;
  std::size_t pool_size_;
  std::atomic<bool> stopped_;
};

}  // namespace foxhttp::server
