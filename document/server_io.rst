.. _server_io:

Server and I/O
==============

IoContextPool
-------------

``foxhttp::server::IoContextPool`` owns one or more ``boost::asio::io_context`` instances used to run asynchronous operations.

* Call ``run_blocking()`` (or ``start()``—same blocking behavior) on a worker thread to run the pool until shutdown.
* Call ``stop()`` from another thread to initiate shutdown.

Typical pattern: start the pool in a ``std::thread``, run the server, then ``stop()`` the pool on application exit.

Server and SslServer
--------------------

* **Server**: plain HTTP listener; constructs with an ``foxhttp::server::IoContextPool`` and port.
* **SslServer**: HTTPS when built with ``FOXHTTP_ENABLE_TLS``; requires TLS configuration (certificates, etc.—see ``foxhttp/config/tls.hpp`` and examples).

Both expose middleware registration via ``global_chain()`` so you attach shared middleware before accepting connections.

Accept and per-connection HTTP/TLS/WebSocket I/O use **Boost.Asio coroutines** (``co_spawn``, ``co_await`` with ``use_awaitable``). See :ref:`coroutines`.

SignalSet
---------

``foxhttp::server::SignalSet`` integrates POSIX-style signals (e.g. ``SIGINT``, ``SIGTERM``) with Asio so you can stop the server or pool cleanly from a signal handler context.

WebSocket
---------

* **WsSession**: WebSocket over plain HTTP upgrade path.
* **WssSession**: secure WebSocket when TLS is enabled.

Use these when you need bidirectional framing on top of the same Asio infrastructure as HTTP.
