# FoxHttp examples

Small programs that show how to use the library end-to-end. Build with:

```bash
cmake -S .. -B ../build -DFOXHTTP_BUILD_EXAMPLES=ON
cmake --build ../build
```

Executables are created under your build directory (e.g. `build/examples/01_hello_server`).

| Example | Port | What it demonstrates |
|--------|------|----------------------|
| `01_hello_server` | 8080 | `foxhttp::server::IoContextPool`, `foxhttp::server::Server`, `foxhttp::server::SignalSet`, `foxhttp::router::Router::register_static_handler`, terminal **router dispatch** middleware |
| `02_routing` | 8081 | Static + dynamic routes, path parameters on `foxhttp::server::RequestContext` |
| `03_middleware_chain` | 8082 | CORS (runs first via `first_cors_middleware`), `foxhttp::middleware::factories::create_simple_logger`, `foxhttp::middleware::ResponseTimeMiddleware`, dispatch |
| `04_static_files` | 8083 | `foxhttp::middleware::StaticMiddleware` for `/static/` + API route; WWW root is `examples/www` (via `FOXHTTP_EXAMPLE_WWW_DIR`) |
| `05_json_body` | 8084 | `foxhttp::middleware::BodyParserMiddleware`, POST JSON echo via `parsed_body<nlohmann::json>()` |
| `07_graceful_shutdown` | 8086 | `foxhttp::server::Server::stop()` for graceful shutdown, signal handling, slow request completion |
| `08_error_handling` | 8087 | Session error callback mechanism, exception logging, error statistics |
| `09_client_timeout` | — | `foxhttp::client::HttpClient` timeout configuration: client-wide, per-request, connection vs request timeouts |

## Router dispatch

HTTP sessions run the global middleware chain with `execute_async` (invoked from an internal **coroutine**; your middleware API is unchanged). The last middleware should resolve `foxhttp::router::Router` handlers and fill the response. Shared helper: `examples/detail/router_dispatch_middleware.hpp` (namespace `foxhttp::examples`). It uses **`foxhttp::middleware::MiddlewarePriority::Highest`** so it runs **after** other middleware once the chain sorts by priority (ascending numeric order).

## Middleware order

`foxhttp::middleware::MiddlewareChain` **sorts** middleware by `priority()` unless you rely on equal priorities. Lower numeric values run first. Stock **CORS** uses `High`; **response_time** uses `Low`. Example 03 uses **`first_cors_middleware`** (`Lowest`) so CORS headers are applied before logger and response-time measurement.

## Try with curl

```bash
./build/examples/01_hello_server
curl -s http://127.0.0.1:8080/health
curl -s http://127.0.0.1:8081/api/users/42
curl -i  http://127.0.0.1:8082/api/hello
curl -s http://127.0.0.1:8083/static/index.html
curl -s -X POST http://127.0.0.1:8084/api/echo -H "Content-Type: application/json" -d '{"x":1}'
```

Stop each server with **Ctrl+C** (SIGINT).
