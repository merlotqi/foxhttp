.. _architecture:

Architecture overview
=====================

High-level flow
---------------

1. **Listen**: ``foxhttp::server::Server`` (or ``foxhttp::server::SslServer``) accepts TCP (TLS) connections.
2. **Session**: Each connection is handled by a **session** object that reads HTTP requests.
3. **Middleware**: The request passes through ``foxhttp::middleware::MiddlewareChain`` (typically the server's ``global_chain()``). Middleware may short-circuit the response (e.g. static file, CORS preflight) or call ``next()`` to continue.
4. **Routing**: After the chain, the server resolves a handler via ``foxhttp::router::Router::resolve_route(path, foxhttp::server::RequestContext &)``.
5. **Handler**: Your ``foxhttp::handler::ApiHandler`` (often a lambda wrapped by ``foxhttp::handler::make_api_handler``) writes ``boost::beast::http::response<boost::beast::http::string_body>``.

Concurrency model
-----------------

* Work is driven by **Boost.Asio**; handlers run on thread(s) associated with an ``boost::asio::io_context``.
* ``foxhttp::server::IoContextPool`` distributes load across multiple ``io_context``\ s (strand pools and examples may refine this further).
* ``foxhttp::router::RouteTable`` uses a **shared mutex** so many readers can resolve routes concurrently; registration mutates under a unique lock.

Middleware execution
--------------------

* The chain **snapshots** the middleware list at execute time.
* Synchronous path: each middleware receives a ``next`` callable; not calling ``next()`` stops the rest of the pipeline for that request.
* Asynchronous path: middlewares can use ``foxhttp::middleware::async_middleware_callback`` with ``foxhttp::middleware::MiddlewareResult`` (``Continue``, ``Stop``, ``Error``, ``Timeout``).

Keep-alive and limits
---------------------

Session behavior (timeouts, body/header limits, keep-alive, max requests per connection) is configured through **session limits** and related structures in ``foxhttp::config`` (``configs.hpp``)—see :doc:`config`.
