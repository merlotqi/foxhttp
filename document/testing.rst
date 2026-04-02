.. _testing:

Unit tests
==========

The ``tests/`` directory contains **GoogleTest** executables, one per ``*.cpp`` file, discovered by CMake's ``gtest_discover_tests``. The project is built as **C++20** (library and tests).

Enable
------

.. code-block:: bash

   cmake -S . -B build -DFOXHTTP_BUILD_TESTS=ON
   cmake --build build
   ctest --test-dir build --output-on-failure

Root ``CMakeLists.txt`` calls ``enable_testing()`` when tests are enabled so ``ctest`` from the **build root** discovers all cases.

Custom target
-------------

``foxhttp_run_tests`` runs ``ctest --output-on-failure`` from the binary directory.

Coverage areas (non-exhaustive)
--------------------------------

* **Router**: static/dynamic resolution, invalid patterns, static-vs-dynamic preference, dynamic route specificity.
* **RequestContext** (``foxhttp::server::RequestContext``): path/query, path parameters, context bag.
* **MiddlewareChain**: ordering, ``next()`` semantics.
* **StaticMiddleware**: file serving, HEAD, delegation, payload limit, directory index.
* **BodyParserMiddleware**: JSON/form/text, skip rules, 400 on bad JSON.
* **CorsMiddleware**, **JsonParser**, **MiddlewareBuilder**, **ResponseTimeMiddleware**.

Benchmarks (Google Benchmark)
-----------------------------

Enable with ``-DFOXHTTP_BUILD_BENCHMARKS=ON``. Google Benchmark is fetched via **FetchContent** (same pattern as GTest). Sources live under ``benchmark/*.cpp``; each file becomes its own executable linked to ``benchmark::benchmark_main`` and ``foxhttp::foxhttplib``. CTest registers them with label ``benchmark``; run ``ctest -L benchmark`` or ``cmake --build build --target foxhttp_run_benchmarks``. See ``benchmark/README.md``.

Microbenchmark areas (see table in ``benchmark/README.md``):

* **Router / context**: static and dynamic resolution, not-found, scaling static map size, dynamic list scan (decoys + match), static preferred over dynamic, ``foxhttp::server::RequestContext`` with query + cookies.
* **JSON**: small document, medium array size, deeply nested object, ``supports()`` for JSON vs wrong ``Content-Type``.
* **Middleware**: empty pipeline vs many synchronous pass-through layers.
* **Form / plaintext**: URL-encoded field count, plaintext body size, ``supports()`` checks.
* **Coroutine bridge**: ``tests/await_middleware_async_test.cpp`` — ``foxhttp::detail::await_middleware_chain_async`` with sync middleware, async ``asio::post`` completion, and empty chain.
* **Coroutine throughput** (benchmark): ``benchmark/coroutine_throughput_benchmark`` — middleware-await microbench and loopback session RPS (see ``benchmark/README.md``).

Adding tests
------------

Add a new ``tests/your_feature_test.cpp``; it is picked up automatically by the glob in ``tests/CMakeLists.txt``. Link only ``GTest::gtest_main`` and ``foxhttp::foxhttplib``.
