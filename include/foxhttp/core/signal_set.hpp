#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace foxhttp {

class signal_set : public std::enable_shared_from_this<signal_set>
{
public:
    using signal_handler = std::function<void(int, const std::string &)>;
    using error_handler = std::function<void(const std::error_code &, const std::string &context)>;

    signal_set(boost::asio::io_context &io_context);
    void register_handler(int signal_number, signal_handler handler);
    void set_error_handler(error_handler handler);

    void start();
    void stop();

private:
    void _do_async_wait();
    void _handle_signal(int sig);
    static std::string _siganl_name(int sig);

private:
    boost::asio::signal_set signals_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    std::mutex mutex_;
    std::unordered_map<int, std::vector<signal_handler>> signal_handlers_;
    error_handler error_handler_;
    std::atomic<bool> stopped_;
};


}// namespace foxhttp