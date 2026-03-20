# FoxHttp

C++17 HTTP server library built on **Boost.Beast / Boost.Asio**: middleware pipeline, routing helpers, parsers (JSON, form, multipart), optional **TLS** and **WebSocket**, YAML-based config helpers.

Licensed under **GPLv3** (see file headers).

## Dependencies

- Boost (system, json, iostreams), OpenSSL (when `FOXHTTP_ENABLE_TLS=ON`), ZLIB, **spdlog**, **nlohmann_json**, **yaml-cpp**

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

CMake options:

| Option | Default | Meaning |
|--------|---------|---------|
| `FOXHTTP_BUILD_EXAMPLES` | `OFF` | Build programs under `examples/` |
| `FOXHTTP_ENABLE_TLS` | `ON` | HTTPS / WSS (`USING_TLS`) |
| `FOXHTTP_ENABLE_JWT` | `OFF` | JWT-related auth middleware |
| `BUILD_WITH_BROTLI` | `OFF` | Brotli compression support |

Install (optional):

```bash
cmake --install build --prefix /path/to/prefix
```

## Usage sketch

Include the umbrella header or granular headers under `include/foxhttp/`.

- **`io_context_pool`**: call `run_blocking()` (or `start()`, same behavior) on a worker thread; call `stop()` from elsewhere to shut down.
- **`server` / `ssl_server`**: register middleware on `global_chain()`, then accept connections as in the examples.
- **`session_limits`** (`configs.hpp`): timeouts, body/header sizes, **keep-alive** (`enable_keep_alive`, `max_requests_per_connection`).

## Docker

See `Dockerfile` for a minimal Ubuntu-based build environment.
