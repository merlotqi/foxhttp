# FoxHttp 代码仓库整理计划

## 已完成工作

### 1. C++17/C++20 条件编译
- ✅ CMakeLists.txt 添加 `FOXHTTP_USE_CPP20` 选项
- ✅ `include/foxhttp/config.hpp` 定义 `FOXHTTP_HAS_COROUTINES` 宏
- ✅ 所有协程相关文件添加条件编译
- ✅ C++17 和 C++20 编译验证通过

### 2. detail 目录统一
- ✅ 重命名 `config/details/` → `config/detail/`
- ✅ 重命名 `parser/details/` → `parser/detail/`
- ✅ 重命名 `src/config/details/` → `src/config/detail/`
- ✅ 更新所有 include 路径
- ✅ C++17 编译验证通过

## 待完成工作

### 3. 类命名规范化 (PascalCase)

**Server 模块:**
- `session` → `Session`
- `session_base` → `SessionBase`
- `ssl_session` → `SslSession`
- `ws_session` → `WsSession`
- `wss_session` → `WssSession`
- `server` → `Server`
- `ssl_server` → `SslServer`

**Client 模块:**
- `http_client` → `HttpClient`
- `request_builder` → `RequestBuilder`
- `client_options` → `ClientOptions`
- `connection_pool` → `ConnectionPool`

**Middleware 模块:**
- `middleware_chain` → `MiddlewareChain`
- `middleware` → `Middleware`
- `cors_middleware` → `CorsMiddleware`
- `logger_middleware` → `LoggerMiddleware`

**Parser 模块:**
- `form_parser` → `FormParser`
- `json_parser` → `JsonParser`
- `multipart_parser` → `MultipartParser`
- `plain_text_parser` → `PlainTextParser`
- `form_field` → `FormField`
- `multipart_field` → `MultipartField`

**Config 结构体:**
- `session_limits` → `SessionLimits`
- `websocket_limits` → `WebsocketLimits`
- `json_config` → `JsonConfig`
- `form_config` → `FormConfig`

### 4. 添加子命名空间

**目标结构:**
```cpp
namespace foxhttp::server { class Server; class Session; }
namespace foxhttp::client { class HttpClient; class RequestBuilder; }
namespace foxhttp::middleware { class Middleware; class MiddlewareChain; }
namespace foxhttp::parser { class FormParser; class JsonParser; }
namespace foxhttp::router { class Router; class Route; }
```

### 5. 向后兼容策略

使用 `using` 别名保留旧名称：
```cpp
namespace foxhttp {
    // 新命名
    class Session { ... };
    
    // 向后兼容 (deprecated)
    using session [[deprecated("Use Session instead")]] = Session;
}
```

## 下一步

1. 完成所有类名 PascalCase 重命名
2. 添加子命名空间
3. 最终编译验证 (C++17 + C++20)
