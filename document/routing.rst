.. _routing:

Routing
=======

The **router** API is static on ``foxhttp::router::Router``; handlers are stored in the singleton **route table** (``foxhttp::router::RouteTable::instance()``).

Registration
------------

* **Static**: ``foxhttp::router::Router::register_static_handler("/path", handler)`` or overload with a lambda; normalized path must match exactly.
* **Dynamic**: ``foxhttp::router::Router::register_dynamic_handler("/api/users/{id}", handler)``; path segments in ``{name}`` become **path parameters** on ``foxhttp::server::RequestContext`` after a successful match.

Handler signature
-----------------

Callable handlers use:

.. code-block:: cpp

   void(foxhttp::server::RequestContext &,
        boost::beast::http::response<boost::beast::http::string_body> &)

Use ``foxhttp::handler::make_api_handler`` / ``foxhttp::handler::CallableApiHandler`` to wrap lambdas into ``std::shared_ptr<foxhttp::handler::ApiHandler>`` when using the non-template registration overloads.

Resolution
----------

.. code-block:: cpp

   auto h = foxhttp::router::Router::resolve_route(path_string, ctx);

* Tries **static** routes first (exact normalized path).
* Then tries **dynamic** routes in **specificity order**: more static-like segments (fewer parameters) are preferred, then longer patterns, then lexicographic tie-break.

Path parameters are written into ``ctx`` by the matching dynamic route.

Macros
------

``REGISTER_STATIC_HANDLER`` and ``REGISTER_DYNAMIC_HANDLER`` (see ``router.hpp``) perform static initialization-time registration for class-based handlers.

Testing and isolation
---------------------

``foxhttp::router::RouteTable::instance().clear()`` removes all routes—useful in unit tests between cases.
