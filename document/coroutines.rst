.. _coroutines:

Coroutines (Boost.Asio)
=======================

FoxHttp uses **C++20 coroutines** with **Boost.Asio** ``boost::asio::awaitable`` and ``boost::asio::co_spawn`` for connection I/O.

What runs as a coroutine
------------------------

* **Accept loop**: ``server`` and ``ssl_server`` spawn a long-lived coroutine that ``co_await``\ s ``async_accept``.
* **HTTP sessions**: ``session`` / ``ssl_session`` run a per-connection coroutine: ``co_await`` ``http::async_read``, then the middleware pipeline, then ``co_await`` ``http::async_write``, with keep-alive looping as before.
* **TLS**: ``ssl_session`` performs the server handshake with ``co_await`` ``async_handshake`` before the HTTP loop.
* **WebSocket**: ``ws_session`` / ``wss_session`` use ``co_await`` for ``async_accept`` on the WebSocket stream and for the read/write echo loop.

Middleware API (unchanged)
--------------------------

Application middleware still implements the existing synchronous ``next()`` / asynchronous ``async_middleware_callback`` interface. The session coroutine **bridges** to ``middleware_chain::execute_async`` via ``foxhttp::detail::await_middleware_chain_async`` (see ``include/foxhttp/detail/await_middleware_async.hpp``): the completion handler is always submitted with ``asio::post`` so synchronous pipeline completion still resumes the coroutine safely.

Timeouts
--------

Header/body read timeouts cancel the pending ``async_read`` on the socket (or lowest layer for TLS). The session coroutine treats ``operation_aborted`` together with an internal reason flag and may send a 408 response, then exit the loop.

Requirements
--------------

* **C++20** toolchain (``co_await``, ``co_return``).
* **Boost** with Beast/Asio supporting ``use_awaitable`` on the async operations used above (typical **Boost ≥ 1.75** setups).
