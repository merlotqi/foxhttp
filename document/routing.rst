.. _routing:

Routing
=======

The **router** API is static; handlers are stored in the singleton **route_table**.

Registration
------------

* **Static**: ``router::register_static_handler("/path", handler)`` or overload with a lambda; normalized path must match exactly.
* **Dynamic**: ``router::register_dynamic_handler("/api/users/{id}", handler)``; path segments in ``{name}`` become **path parameters** on ``request_context`` after a successful match.

Handler signature
-----------------

Callable handlers use:

.. code-block:: cpp

   void(foxhttp::request_context &,
        boost::beast::http::response<boost::beast::http::string_body> &)

Use ``make_api_handler`` / ``callable_api_handler`` to wrap lambdas into ``std::shared_ptr<api_handler>`` when using the non-template registration overloads.

Resolution
----------

.. code-block:: cpp

   auto h = foxhttp::router::resolve_route(path_string, ctx);

* Tries **static** routes first (exact normalized path).
* Then tries **dynamic** routes in **specificity order**: more static-like segments (fewer parameters) are preferred, then longer patterns, then lexicographic tie-break.

Path parameters are written into ``ctx`` by the matching ``dynamic_route``.

Macros
------

``REGISTER_STATIC_HANDLER`` and ``REGISTER_DYNAMIC_HANDLER`` (see ``router.hpp``) perform static initialization-time registration for class-based handlers.

Testing and isolation
---------------------

``route_table::clear()`` removes all routes—useful in unit tests between cases.
