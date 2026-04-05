#include <gtest/gtest.h>

#include <atomic>
#include <boost/asio/io_context.hpp>
#include <exception>
#include <foxhttp/foxhttp.hpp>
#include <memory>
#include <stdexcept>

class MockSessionBase : public foxhttp::session_base {
 public:
  MockSessionBase(boost::asio::io_context& ioc, std::shared_ptr<foxhttp::middleware_chain> chain)
      : session_base(ioc.get_executor(), chain) {}

  void trigger_error(const std::exception_ptr& eptr) { notify_error(eptr); }
};

TEST(CoroutineException, ErrorCallbackCanBeSet) {
  boost::asio::io_context ioc;
  auto chain = std::make_shared<foxhttp::middleware_chain>(ioc);
  MockSessionBase session(ioc, chain);

  std::atomic<bool> callback_called{false};

  session.set_error_callback([&](const std::exception_ptr& eptr) {
    callback_called = true;
    EXPECT_NE(eptr, nullptr);
  });

  // Trigger an error
  try {
    throw std::runtime_error("test error");
  } catch (...) {
    session.trigger_error(std::current_exception());
  }

  EXPECT_TRUE(callback_called);
}

TEST(CoroutineException, ErrorCallbackReceivesCorrectException) {
  boost::asio::io_context ioc;
  auto chain = std::make_shared<foxhttp::middleware_chain>(ioc);
  MockSessionBase session(ioc, chain);

  std::string exception_message;

  session.set_error_callback([&](const std::exception_ptr& eptr) {
    try {
      if (eptr) {
        std::rethrow_exception(eptr);
      }
    } catch (const std::runtime_error& e) {
      exception_message = e.what();
    }
  });

  // Trigger an error with a specific message
  try {
    throw std::runtime_error("specific error message");
  } catch (...) {
    session.trigger_error(std::current_exception());
  }

  EXPECT_EQ(exception_message, "specific error message");
}

TEST(CoroutineException, ErrorCallbackNotCalledWhenNotSet) {
  boost::asio::io_context ioc;
  auto chain = std::make_shared<foxhttp::middleware_chain>(ioc);
  MockSessionBase session(ioc, chain);

  // Don't set error callback

  // Trigger an error - should not crash
  try {
    throw std::runtime_error("test error");
  } catch (...) {
    session.trigger_error(std::current_exception());
  }

  // No crash means success
  SUCCEED();
}

TEST(CoroutineException, ErrorCallbackCanBeReplaced) {
  boost::asio::io_context ioc;
  auto chain = std::make_shared<foxhttp::middleware_chain>(ioc);
  MockSessionBase session(ioc, chain);

  std::atomic<int> callback1_count{0};
  std::atomic<int> callback2_count{0};

  // Set first callback
  session.set_error_callback([&](const std::exception_ptr&) { callback1_count++; });

  // Trigger error
  try {
    throw std::runtime_error("test error");
  } catch (...) {
    session.trigger_error(std::current_exception());
  }

  EXPECT_EQ(callback1_count, 1);
  EXPECT_EQ(callback2_count, 0);

  // Replace with second callback
  session.set_error_callback([&](const std::exception_ptr&) { callback2_count++; });

  // Trigger error again
  try {
    throw std::runtime_error("test error");
  } catch (...) {
    session.trigger_error(std::current_exception());
  }

  EXPECT_EQ(callback1_count, 1);
  EXPECT_EQ(callback2_count, 1);
}

TEST(CoroutineException, ErrorCallbackCanHandleNullExceptionPtr) {
  boost::asio::io_context ioc;
  auto chain = std::make_shared<foxhttp::middleware_chain>(ioc);
  MockSessionBase session(ioc, chain);

  std::atomic<bool> callback_called{false};

  session.set_error_callback([&](const std::exception_ptr& eptr) {
    callback_called = true;
    // Should handle null exception_ptr gracefully
    if (eptr) {
      try {
        std::rethrow_exception(eptr);
      } catch (...) {
        // Exception handling
      }
    }
  });

  // Trigger with null exception_ptr
  session.trigger_error(nullptr);

  EXPECT_TRUE(callback_called);
}
