FoxHttp documentation
=====================

FoxHttp is a **C++20** HTTP server library built on **Boost.Beast** and **Boost.Asio**. It provides a middleware pipeline, static and parameterized routing, body parsers (JSON, form, multipart, plain text), and optional **TLS**, **WebSocket**, compression, rate limiting, and security headers. The server stack uses **Asio coroutines** for accept/read/write (see :ref:`coroutines`).

.. note::

   This manual describes concepts and usage. For per-symbol API details, use **Doxygen** (see ``doxygen/``) or read the headers under ``include/foxhttp/``.

.. toctree::
   :maxdepth: 2
   :caption: Contents

   intro
   build_install
   architecture
   coroutines
   server_io
   routing
   middleware
   parsers
   request_context
   config
   packaging
   testing

Indices and tables
------------------

* :ref:`genindex`
* :ref:`search` (when built with Sphinx)
