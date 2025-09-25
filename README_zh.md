# FoxHttp

一个基于 Boost.Asio 构建的现代化高性能 C++ HTTP 服务器框架。

## 项目概述

FoxHttp 是一个功能丰富的 HTTP 服务器框架，专为高性能和可扩展性设计。它提供了完整的 Web 服务开发所需的核心功能，包括中间件系统、会话管理、高级请求解析器等。

### 核心特性

- **高性能**：基于 Boost.Asio 构建，具有高效的 I/O 处理能力
- **中间件系统**：灵活可扩展的中间件架构
- **会话管理**：内置会话支持，与中间件无缝集成
- **高级解析器**：支持 multipart、JSON、表单数据和纯文本解析
- **Strand 池**：支持多种负载均衡策略的高级 Strand 池
- **定时器管理**：具有负载均衡功能的高效定时器处理
- **便捷配置**：基于 YAML 的配置系统
- **丰富示例**：提供所有主要功能的即用型示例

## 前置条件

- C++17 或更高版本
- Boost.Asio
- CMake 3.10 或更高版本
- spdlog
- nlohmann_json
- yaml-cpp
- gflags

## 构建方法

```bash
mkdir build && cd build
cmake ..
make
```

## 运行方法

### 基本用法

```cpp
#include <foxhttp/foxhttp.hpp>

int main() {
    foxhttp::io_context_pool io_pool;
    foxhttp::server server(io_pool, 8080);
    io_pool.start();
    return 0;
}
```

## 文档

详细文档请参考 `docs/` 目录：

- [中间件系统](docs/MIDDLEWARE_DESIGN_REFACTOR.md)
- [会话集成](docs/SESSION_MIDDLEWARE_INTEGRATION.md)
- [高级 Strand 池](docs/ADVANCED_STRAND_POOL.md)
- [定时器管理](docs/TIMER_MANAGER_REFACTOR.md)
- [SPDLog 集成](docs/SPDLOG_MIDDLEWARE_GUIDE.md)

## 示例

查看 `examples/` 目录获取全面的使用示例：

- 基本服务器设置
- 中间件实现
- 会话管理
- 请求解析
- 高级功能

## 许可证

本项目采用 MIT 许可证 - 详情请参阅 LICENSE 文件。
