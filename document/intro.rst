.. _intro:

Introduction
============

FoxHttp targets applications that need an **asynchronous HTTP/HTTPS** stack in modern C++, without adopting an entire application framework.

What you get
------------

* **I/O and threading**: ``foxhttp::server::IoContextPool`` runs worker ``boost::asio::io_context``\ s; ``run_blocking()`` (or ``start()``) blocks until ``stop()``.
* **HTTP server**: ``foxhttp::server::Server`` / ``foxhttp::server::SslServer`` accept connections; each session runs the global middleware chain (via ``execute_async``, bridged from a **C++20 coroutine**) then dispatches to your routes.
* **Middleware**: ordered pipeline with priorities, optional timeouts, sync and async completion callbacks.
* **Routing**: ``foxhttp::router::Router`` registers handlers on a process-wide ``foxhttp::router::RouteTable``; static paths and patterns such as ``/api/users/{id}``.
* **Parsers**: pluggable parsers registered in ``foxhttp::parser::ParserFactory``; optional ``foxhttp::middleware::BodyParserMiddleware`` fills ``foxhttp::server::RequestContext`` from ``Content-Type``.
* **Optional features** (CMake-gated): OpenSSL TLS, WebSocket/WSS, Brotli, JWT-related auth middleware.

Licensing
---------

FoxHttp is licensed under **GPLv3**. Ensure your distribution and linking comply with the license.

Headers
-------

Include the umbrella header ``foxhttp/foxhttp.hpp`` for a quick start, or include granular headers under ``include/foxhttp/`` to minimize compile-time dependencies.

Examples
--------

The ``examples/`` directory contains runnable programs. They are the best reference for end-to-end wiring (ports, TLS material, middleware order).
