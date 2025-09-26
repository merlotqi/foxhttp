#include <gflags/gflags.h>
#include <iostream>
#include <foxhttp/foxhttp.hpp>


DEFINE_int32(port, 8080, "Server port number");
DEFINE_string(log_dir, "./logs", "Log directory path");

int main(int argc, char **argv)
{

    gflags::SetUsageMessage("Usage: " + std::string(argv[0]) +
                            " --port=<port> --log_dir=<path>\n"
                            "Example:\n"
                            "  " +
                            std::string(argv[0]) + " --port=8080 --log_dir=./logs");
    
        
    try
    {
        gflags::ParseCommandLineFlags(&argc, &argv, true);
        if (FLAGS_port <= 0 || FLAGS_log_dir.empty())
        {
            std::cerr << "Error: --port and --log_dir are required.\n\n";
            std::cerr << gflags::ProgramUsage();
            return 1;
        }

        std::cout << "Server will start on port: " << FLAGS_port << std::endl;
        std::cout << "Log directory: " << FLAGS_log_dir << std::endl;

        foxhttp::io_context_pool io_pool;
        foxhttp::server server(io_pool, FLAGS_port);
        io_pool.start();

    }catch(std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }

    return 0;
}