.. _http_client:

HTTP client
===========

FoxHttp includes a small **HTTP/HTTPS client** in namespace ``foxhttp::client``, built on **Boost.Beast** and **Boost.Asio** with the same coroutine style as the server stack.

Include either ``<foxhttp/client/http_client.hpp>`` or the umbrella header ``<foxhttp/client.hpp>``.

Construction
------------

* **HTTP:** ``http_client(io_executor, "http://host:port")`` — base URL must use the ``http://`` scheme (default port 80 if omitted).
* **HTTPS** (when the library is built with TLS, ``FOXHTTP_ENABLE_TLS``): ``http_client(io_executor, "https://host:port", ssl_context)``. Requests that use ``https://`` (absolute URL or HTTPS base) require this constructor so a client TLS context is available.

Fluent requests
---------------

Obtain a ``request_builder`` from ``get`` / ``post`` / ``put`` / ``delete_``. You can pass a **path** (joined with the client base URL) or an **absolute** ``http://`` / ``https://`` URL.

Chain modifiers:

* ``header(name, value)``, ``body(string)``, ``body_json(nlohmann::json)``
* Optional ``catch_error(fn)`` — register **before** ``then`` if you use it; receives ``boost::system::error_code`` on connection, TLS, or I/O failure.
* ``then(fn)`` — starts the request (``co_spawn`` on the client executor) and invokes ``fn`` with ``boost::beast::http::response<boost::beast::http::string_body>`` on success. HTTP 4xx/5xx are still delivered to ``then`` unless you treat them in your callback.

Coroutines
----------

From a coroutine running on an Asio executor (typically the same ``io_context`` as the client), use::

   co_await client.get("/path").as_awaitable();

On failure, ``as_awaitable`` completes with an exception (``boost::system::system_error``).

**Do not** call ``then`` on the same builder after ``as_awaitable``, or vice versa: each builder is **single-use**. The library throws ``std::logic_error`` if you try to reuse it.

Threading
---------

Callbacks from ``then`` / ``catch_error`` are **posted** to the client’s executor, so they run in whatever thread is executing that executor’s ``io_context::run`` (or equivalent). Avoid blocking work there.

HTTPS and keep-alive
--------------------

* **HTTPS** uses ``boost::beast::ssl_stream`` over a TCP stream when the effective URL is HTTPS and a client ``ssl::context`` was supplied.
* **Connection pooling / keep-alive** is not implemented yet; each request uses a separate connection and closes it when done.
