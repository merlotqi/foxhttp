.. _intro:

Introduction
============

FoxHttp targets applications that need an **asynchronous HTTP/HTTPS** stack in modern C++, without adopting an entire application framework.

What you get
------------

* **I/O and threading**: ``io_context_pool`` runs worker ``io_context``\ s; ``run_blocking()`` (or ``start()``) blocks until ``stop()``.
* **HTTP server**: ``server`` / ``ssl_server`` accept connections; each session runs the global middleware chain then dispatches to your routes.
* **Middleware**: ordered pipeline with priorities, optional timeouts, sync and async completion callbacks.
* **Routing**: ``router`` registers handlers on a process-wide ``route_table``; static paths and patterns such as ``/api/users/{id}``.
* **Parsers**: pluggable parsers registered in ``parser_factory``; optional ``body_parser_middleware`` fills ``request_context`` from ``Content-Type``.
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
