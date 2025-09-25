# FoxHttp

A modern, high-performance C++ HTTP server framework built with Boost.Asio.

## Features

- **High Performance**: Built on Boost.Asio with efficient I/O handling
- **Middleware System**: Flexible and extensible middleware architecture
- **Session Management**: Built-in session support with middleware integration
- **Advanced Parsers**: Support for multipart, JSON, form data, and plain text
- **Strand Pool**: Advanced load balancing with multiple strategies
- **Timer Management**: Efficient timer handling with load balancing
- **Easy Configuration**: YAML-based configuration system
- **Comprehensive Examples**: Ready-to-use examples for all major features

## Quick Start

### Building

```bash
mkdir build && cd build
cmake ..
make
```

### Basic Usage

```cpp
#include <foxhttp/foxhttp.hpp>

int main() {
    foxhttp::io_context_pool io_pool;
    foxhttp::server server(io_pool, 8080);
    io_pool.start();
    return 0;
}
```

## Documentation

Detailed documentation is available in the `docs/` directory:

- [Middleware System](docs/MIDDLEWARE_DESIGN_REFACTOR.md)
- [Session Integration](docs/SESSION_MIDDLEWARE_INTEGRATION.md)
- [Advanced Strand Pool](docs/ADVANCED_STRAND_POOL.md)
- [Timer Manager](docs/TIMER_MANAGER_REFACTOR.md)
- [SPDLog Integration](docs/SPDLOG_MIDDLEWARE_GUIDE.md)

## Examples

Check the `examples/` directory for comprehensive usage examples:

- Basic server setup
- Middleware implementation
- Session management
- Request parsing
- Advanced features

## Requirements

- C++17 or later
- Boost.Asio
- CMake 3.10 or higher
- spdlog
- nlohmann_json
- yaml-cpp
- gflags

## License

This project is licensed under the MIT License - see the LICENSE file for details.
