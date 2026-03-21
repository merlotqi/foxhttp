.. _request_context:

Request context
===============

``request_context`` wraps the incoming ``boost::beast::http::request<boost::beast::http::string_body>`` and adds FoxHttp-specific state.

HTTP surface
------------

* ``method()``, ``path()`` (path without query), ``query()``, ``body()``
* ``header(name_or_field, default)``
* ``query_parameters()``—multimap-like structure for repeated keys

Path parameters
---------------

Set by the router for dynamic routes: ``path_parameter("id")``, ``contains_path_parameter``, ``path_parameters()``.

Per-request bag
---------------

* ``set<T>(key, value)`` / ``get<T>(key, default)`` / ``has(key)``—thread-safe ``std::any`` storage for arbitrary request-scoped data (e.g. request IDs from logging middleware).

Parsed body cache
-----------------

* ``set_parsed_body<T>(value)`` / ``parsed_body<T>()``—requires ``T`` to be suitable for ``std::any`` (copyable). For **form** and **multipart** bodies, prefer the **shared_ptr** keys documented in :doc:`parsers`.

Cookies
-------

Parsed from the ``Cookie`` header: ``cookie(name)``, ``cookies()``.
