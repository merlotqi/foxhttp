.. _server_io:

Server and I/O
==============

io_context_pool
---------------

``io_context_pool`` owns one or more ``boost::asio::io_context`` instances used to run asynchronous operations.

* Call ``run_blocking()`` (or ``start()``—same blocking behavior) on a worker thread to run the pool until shutdown.
* Call ``stop()`` from another thread to initiate shutdown.

Typical pattern: start the pool in a ``std::thread``, run the server, then ``stop()`` the pool on application exit.

server and ssl_server
---------------------

* **server**: plain HTTP listener; constructs with an address/port and an ``io_context_pool``.
* **ssl_server**: HTTPS when built with ``FOXHTTP_ENABLE_TLS``; requires TLS configuration (certificates, etc.—see ``config/tls.hpp`` and examples).

Both expose middleware registration via ``global_chain()`` (or equivalent) so you attach shared middleware before accepting connections.

signal_set
----------

``signal_set`` integrates POSIX-style signals (e.g. ``SIGINT``, ``SIGTERM``) with Asio so you can stop the server or pool cleanly from a signal handler context.

WebSocket
---------

* **ws_session**: WebSocket over plain HTTP upgrade path.
* **wss_session**: secure WebSocket when TLS is enabled.

Use these when you need bidirectional framing on top of the same Asio infrastructure as HTTP.
