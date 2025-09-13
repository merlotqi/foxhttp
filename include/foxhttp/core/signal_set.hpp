#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace foxhttp {

class SignalSet : public std::enable_shared_from_this<SignalSet>
{
public:
    using SignalHandler = std::function<void(int, const std::string &)>;
    using ErrorHandler = std::function<void(const std::error_code &, const std::string &context)>;

    SignalSet(boost::asio::io_context &io_context);
    void register_handler(int signal_number, SignalHandler handler);
    void set_error_handler(ErrorHandler handler);

    void start();
    void stop();

private:
    void do_async_wait();
    void handle_signal(int sig);
    static std::string siganl_name(int sig);

private:
    boost::asio::signal_set signals_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    std::mutex mutex_;
    std::unordered_map<int, std::vector<SignalHandler>> signal_handlers_;
    ErrorHandler error_handler_;
    std::atomic<bool> stopped_;
};


}// namespace foxhttp