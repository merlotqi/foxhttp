# FoxHttp

C++17 HTTP server library built on **Boost.Beast / Boost.Asio**: middleware pipeline, routing helpers, parsers (JSON, form, multipart), optional **TLS** and **WebSocket**, YAML-based config helpers.

Licensed under **GPLv3**.

## CI/CD

[![CI](https://github.com/merlotqi/foxhttp/actions/workflows/ci.yml/badge.svg)](https://github.com/merlotqi/foxhttp/actions/workflows/ci.yml)

- **CI** ([`.github/workflows/ci.yml`](.github/workflows/ci.yml)): Ubuntu 22.04, CMake **Release** build with tests, examples, and benchmarks; full `ctest`.
- **Docker** ([`.github/workflows/docker.yml`](.github/workflows/docker.yml)): builds the `Dockerfile` when it changes (PR or push to `main`).
- **CD** ([`.github/workflows/cd-docker.yml`](.github/workflows/cd-docker.yml)): on git tags matching `v*`, builds and pushes `ghcr.io/merlotqi/foxhttp:<tag>` and `:latest` (repository must allow **Packages** write for `GITHUB_TOKEN`).

## Dependencies

- **Boost ≥ 1.75** (components: system, **json**, iostreams — Boost.JSON is not in 1.74), OpenSSL (when `FOXHTTP_ENABLE_TLS=ON`), ZLIB, **spdlog**, **nlohmann_json**, **yaml-cpp**

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

With tests enabled, run `ctest --test-dir build --output-on-failure` (or `cmake --build build --target foxhttp_run_tests`). With benchmarks enabled, run `ctest --test-dir build -L benchmark` or `cmake --build build --target foxhttp_run_benchmarks` (see [`benchmark/README.md`](benchmark/README.md)).

CMake options:

| Option | Default | Meaning |
|--------|---------|---------|
| `FOXHTTP_BUILD_EXAMPLES` | `OFF` | Build programs under `examples/` |
| `FOXHTTP_BUILD_TESTS` | `OFF` | Unit tests under `tests/` (GoogleTest via FetchContent) |
| `FOXHTTP_BUILD_BENCHMARKS` | `OFF` | Microbenchmarks under `benchmark/` (Google Benchmark via FetchContent) |
| `FOXHTTP_ENABLE_TLS` | `ON` | HTTPS / WSS (`USING_TLS`) |
| `FOXHTTP_ENABLE_JWT` | `OFF` | JWT-related auth middleware |
| `BUILD_WITH_BROTLI` | `OFF` | Brotli compression support |

Install (optional):

```bash
cmake --install build --prefix /path/to/prefix
```

### Use as `add_subdirectory` / git submodule

From your project root (after `add_subdirectory(third_party/foxhttp)` or similar):

```cmake
target_link_libraries(your_target PRIVATE foxhttp::foxhttplib)
```

FoxHttp runs `find_package` for Boost, ZLIB, spdlog, nlohmann_json, yaml-cpp, and OpenSSL when TLS is on—your toolchain must expose them (e.g. `CMAKE_PREFIX_PATH`, vcpkg, or system packages). The library uses **`FOXHTTP_SOURCE_DIR`** for public include paths so it stays correct when embedded; avoid copying patterns that use **`CMAKE_SOURCE_DIR`** inside a reusable library (that always points at the **top-level** project, not FoxHttp).

The generic option name `BUILD_WITH_BROTLI` can clash with another dependency; if that happens, set it before adding FoxHttp or fork the option name to `FOXHTTP_BUILD_WITH_BROTLI` in a future change.

## Usage sketch

Include the umbrella header or granular headers under `include/foxhttp/`.

- **`io_context_pool`**: call `run_blocking()` (or `start()`, same behavior) on a worker thread; call `stop()` from elsewhere to shut down.
- **`server` / `ssl_server`**: register middleware on `global_chain()`, then accept connections as in the examples.
- **`session_limits`** (`configs.hpp`): timeouts, body/header sizes, **keep-alive** (`enable_keep_alive`, `max_requests_per_connection`).

### Lambda / callable route registration

Static and dynamic routes use the same handler signature: `void(foxhttp::request_context &, boost::beast::http::response<boost::beast::http::string_body> &)`. Dynamic path parameters are available on `ctx` after `resolve_route` (e.g. `ctx.path_parameter("id")`).

```cpp
foxhttp::router::register_static_handler("/api/version",
    [](foxhttp::request_context &ctx, boost::beast::http::response<boost::beast::http::string_body> &res) {
      res.result(boost::beast::http::status::ok);
      res.set(boost::beast::http::field::content_type, "application/json");
      res.body() = R"({"version":"0.1.0"})";
    });

foxhttp::router::register_dynamic_handler("/api/users/{id}",
    [](foxhttp::request_context &ctx, boost::beast::http::response<boost::beast::http::string_body> &res) {
      res.result(boost::beast::http::status::ok);
      res.body() = ctx.path_parameter("id");
    });
```

## Examples

With `-DFOXHTTP_BUILD_EXAMPLES=ON`, see [`examples/README.md`](examples/README.md) for numbered demos (`01_hello_server`, routing, middleware chain, static files, JSON body).

## Documentation

English reStructuredText manual: see the [`document/`](document/) directory (build HTML with Sphinx: `sphinx-build -b html document document/_build/html`).

## Docker

See `Dockerfile` for a minimal Ubuntu-based build environment.
