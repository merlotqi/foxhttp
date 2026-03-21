# FoxHttp microbenchmarks

Uses [Google Benchmark](https://github.com/google/benchmark), fetched with **CMake FetchContent** (same idea as GTest in `tests/`).

## Configure & build

```bash
cmake -S .. -B ../build -DFOXHTTP_BUILD_BENCHMARKS=ON
cmake --build ../build
```

Binaries appear under `build/benchmark/` (one executable per `*.cpp`):

| Binary | Rough coverage |
|--------|----------------|
| `router_benchmark` | Static/dynamic resolve, miss path, large static table, dynamic scan depth, static-over-dynamic, `request_context` construction |
| `json_parse_benchmark` | Small/medium/deep JSON payloads, `supports()` hot paths |
| `middleware_chain_benchmark` | Empty `execute()`, pass-through depth (noop functional middlewares) |
| `form_plaintext_benchmark` | `application/x-www-form-urlencoded` parse (field count), plaintext body |

## Run

Google Benchmark **v1.8.x** expects ``--benchmark_min_time`` as a **seconds** value with an ``s`` suffix (e.g. ``0.01s``), or as ``<float>x`` for iterations—not ``10ms``.

```bash
./build/benchmark/router_benchmark
./build/benchmark/json_parse_benchmark --benchmark_min_time=0.1s --benchmark_out=results.json --benchmark_format=json
```

Via CTest (short runs; label `benchmark`):

```bash
ctest --test-dir build -L benchmark --output-on-failure
```

Or the aggregate target:

```bash
cmake --build build --target foxhttp_run_benchmarks
```

## Notes

- Benchmark sources must **not** define `BENCHMARK_MAIN()` when linking `benchmark::benchmark_main`.
- Router benchmarks clear `route_table` around each suite to avoid cross-case pollution.
