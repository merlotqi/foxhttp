.. _middleware:

Middleware
==========

Concepts
--------

* Derive from ``middleware`` or use **functional_middleware** / **middleware_builder**.
* Implement ``operator()(request_context &, response &, std::function<void()> next)``.
* Optionally override ``priority()``, ``name()``, ``should_execute()``, ``timeout()``.
* **Priority**: the chain **sorts** middleware by ascending numeric ``priority()`` (smaller runs first). Place terminal logic (e.g. router dispatch) on a middleware with **high** priority value such as ``middleware_priority::highest`` so it runs **last**. Stock **CORS** uses ``high`` and **response_time** uses ``low``, so CORS may run *after* response-time unless you wrap or subclass—see the ``first_cors_middleware`` pattern in ``examples/detail/``.

middleware_chain
----------------

* ``use()`` registers middleware; the chain may **auto-sort** by priority.
* ``execute()`` runs the pipeline synchronously for a single request/response pair.
* ``execute_async()`` runs with completion callbacks and supports global/middleware timeouts when an ``io_context`` is associated.

Stopping the pipeline
-----------------------

* If a middleware **does not** call ``next()``, later middleware and the default route dispatch may not run—use this when the response is complete (e.g. static file served, CORS preflight).

Basic middleware (examples)
---------------------------

* **cors_middleware**: CORS headers; OPTIONS preflight responds without calling ``next()``.
* **logger_middleware**: structured or Apache-style access logging via spdlog.
* **response_time_middleware**: adds a duration header after ``next()`` returns.
* **static_middleware**: serves files under a URL prefix from a document root (GET/HEAD); path traversal outside the root is rejected.
* **body_parser_middleware**: parses body by ``Content-Type`` using ``parser_factory``; see :doc:`parsers`.

Advanced middleware
-------------------

* **compression_middleware**, **rate_limit_middleware**, **security_headers_middleware**, **auth_middleware** (JWT when enabled).

Factories
---------

``middleware_factories`` provides small helpers (e.g. ``create_cors_middleware``) returning ``std::shared_ptr<middleware>``.

Conditional execution
---------------------

* **functional_middleware** supports a **condition** functor exposed as ``should_execute()``—the **middleware_chain** checks this **before** invoking the middleware body. Calling ``operator()`` directly bypasses that check.

* **conditional_middleware** wraps another middleware and a predicate.
