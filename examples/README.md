# FoxHttp examples

Small programs that show how to use the library end-to-end. Build with:

```bash
cmake -S .. -B ../build -DFOXHTTP_BUILD_EXAMPLES=ON
cmake --build ../build
```

Executables are created under your build directory (e.g. `build/examples/01_hello_server`).

| Example | Port | What it demonstrates |
|--------|------|----------------------|
| `01_hello_server` | 8080 | `io_context_pool`, `server`, `signal_set`, `router::register_static_handler`, terminal **router dispatch** middleware |
| `02_routing` | 8081 | Static + dynamic routes, `path_parameter` |
| `03_middleware_chain` | 8082 | CORS (runs first via `first_cors_middleware`), logger, `response_time_middleware`, dispatch |
| `04_static_files` | 8083 | `static_middleware` for `/static/` + API route; WWW root is `examples/www` (via `FOXHTTP_EXAMPLE_WWW_DIR`) |
| `05_json_body` | 8084 | `body_parser_middleware`, POST JSON echo via `parsed_body<nlohmann::json>()` |

## Router dispatch

HTTP sessions run the global middleware chain with `execute_async` (invoked from an internal **coroutine**; your middleware API is unchanged). The last middleware should resolve `router::` handlers and fill the response. Shared helper: `examples/detail/router_dispatch_middleware.hpp` (namespace `foxhttp::examples`). It uses **`middleware_priority::highest`** so it runs **after** other middleware once the chain sorts by priority (ascending numeric order).

## Middleware order

`middleware_chain` **sorts** middleware by `priority()` unless you rely on equal priorities. Lower numeric values run first. Stock **CORS** uses `high`; **response_time** uses `low`. Example 03 uses **`first_cors_middleware`** (`lowest`) so CORS headers are applied before logger and response-time measurement.

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
