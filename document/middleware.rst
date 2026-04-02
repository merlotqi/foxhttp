.. _middleware:

Middleware
==========

Concepts
--------

* Derive from ``foxhttp::middleware::Middleware`` or use **foxhttp::middleware::FunctionalMiddleware** / **foxhttp::middleware::MiddlewareBuilder**.
* Implement ``operator()(foxhttp::middleware::RequestContext &, response &, std::function<void()> next)``. (``RequestContext`` is a type alias for ``foxhttp::server::RequestContext``.)
* Optionally override ``priority()``, ``name()``, ``should_execute()``, ``timeout()``.
* **Priority**: the chain **sorts** middleware by ascending numeric ``priority()`` (smaller runs first). Place terminal logic (e.g. router dispatch) on a middleware with **high** priority value such as ``foxhttp::middleware::MiddlewarePriority::Highest`` so it runs **last**. Stock **CORS** uses ``High`` and **response_time** uses ``Low``, so CORS may run *after* response-time unless you wrap or subclass—see the ``first_cors_middleware`` pattern in ``examples/detail/``.

MiddlewareChain
---------------

* ``use()`` registers middleware; the chain may **auto-sort** by priority.
* ``execute()`` runs the pipeline synchronously for a single request/response pair.
* ``execute_async()`` runs with completion callbacks and supports global/middleware timeouts when an ``boost::asio::io_context`` is associated.

Stopping the pipeline
---------------------

* If a middleware **does not** call ``next()``, later middleware and the default route dispatch may not run—use this when the response is complete (e.g. static file served, CORS preflight).

Basic middleware (examples)
-------------------------

* **CorsMiddleware**: CORS headers; OPTIONS preflight responds without calling ``next()``.
* **LoggerMiddleware**: structured or Apache-style access logging via spdlog.
* **ResponseTimeMiddleware**: adds a duration header after ``next()`` returns.
* **StaticMiddleware**: serves files under a URL prefix from a document root (GET/HEAD); path traversal outside the root is rejected.
* **BodyParserMiddleware**: parses body by ``Content-Type`` using ``foxhttp::parser::ParserFactory``; see :doc:`parsers`.

Advanced middleware
-------------------

* **CompressionMiddleware**, **RateLimitMiddleware**, **SecurityHeadersMiddleware**, **AuthMiddleware** (JWT when enabled), under ``foxhttp::middleware::advanced`` where applicable.

Factories
---------

``foxhttp::middleware::factories`` provides small helpers (e.g. ``create_cors_middleware``) returning ``std::shared_ptr<foxhttp::middleware::Middleware>``. The nested name ``foxhttp::middleware::middleware_factories`` is deprecated but still aliases the same functions.

Conditional execution
---------------------

* **FunctionalMiddleware** supports a **condition** functor exposed as ``should_execute()``—``foxhttp::middleware::MiddlewareChain`` checks this **before** invoking the middleware body. Calling ``operator()`` directly bypasses that check.

* **ConditionalMiddleware** wraps another middleware and a predicate.
