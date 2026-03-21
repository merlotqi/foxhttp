.. _architecture:

Architecture overview
=====================

High-level flow
---------------

1. **Listen**: ``server`` (or ``ssl_server``) accepts TCP (TLS) connections.
2. **Session**: Each connection is handled by a **session** object that reads HTTP requests.
3. **Middleware**: The request passes through ``middleware_chain`` (typically the server's ``global_chain()``). Middleware may short-circuit the response (e.g. static file, CORS preflight) or call ``next()`` to continue.
4. **Routing**: After the chain, the server resolves a handler via ``router::resolve_route(path, request_context)``.
5. **Handler**: Your ``api_handler`` (often a lambda wrapped by ``make_api_handler``) writes ``boost::beast::http::response<boost::beast::http::string_body>``.

Concurrency model
-----------------

* Work is driven by **Boost.Asio**; handlers run on thread(s) associated with an ``io_context``.
* ``io_context_pool`` distributes load across multiple ``io_context``\ s (strand pools and examples may refine this further).
* ``route_table`` uses a **shared mutex** so many readers can resolve routes concurrently; registration mutates under a unique lock.

Middleware execution
--------------------

* The chain **snapshots** the middleware list at execute time.
* Synchronous path: each middleware receives a ``next`` callable; not calling ``next()`` stops the rest of the pipeline for that request.
* Asynchronous path: middlewares can use ``async_middleware_callback`` with ``middleware_result`` (``continue_``, ``stop``, ``error``, ``timeout``).

Keep-alive and limits
---------------------

Session behavior (timeouts, body/header limits, keep-alive, max requests per connection) is configured through **session_limits** and related structures in ``configs.hpp``—see :doc:`config`.
