.. _parsers:

Parsers and body parsing
=========================

parser<T> and parser_factory
-----------------------------

Parsers implement ``parser<T>``:

* ``name()``, ``content_type()``
* ``supports(request)``—whether this parser accepts the request (usually inspects ``Content-Type``).
* ``parse(request)``—returns ``T``.

``parser_factory::instance()`` holds a registry keyed by ``std::type_index``. Register with ``register_parser<T>(std::shared_ptr<parser<T>>)``; the headers for ``json_parser``, ``form_parser``, etc. use a **REGISTER_PARSER** macro to register default implementations at static initialization.

Built-in parsers
----------------

* **json_parser** → ``nlohmann::json``; configurable limits and validation in ``json_config``.
* **form_parser** → ``form_data`` (map of field names to ``form_field``); ``application/x-www-form-urlencoded``.
* **multipart_parser** → ``multipart_data``; ``multipart/form-data`` with boundary extraction.
* **plain_text_parser** → ``std::string``; ``text/*`` and related types per configuration.

body_parser_middleware
----------------------

Selects a parser based on the **main** ``Content-Type`` (substring before ``;``, case-insensitive):

* ``application/json`` → ``set_parsed_body<nlohmann::json>(...)``
* ``application/x-www-form-urlencoded`` → stores ``std::shared_ptr<form_data>`` under ``body_parser_context_keys::form`` (move-only types cannot be stored in ``std::any`` used by ``set_parsed_body``).
* ``multipart/*`` → ``std::shared_ptr<multipart_data>`` under ``body_parser_context_keys::multipart``.
* ``text/*`` → ``set_parsed_body<std::string>(...)``.

On parse failure, optional **400 Bad Request** response when ``bad_request_on_parse_error`` is true.

Extending
---------

Implement ``parser<YourType>``, register it in ``parser_factory``, and optionally extend ``body_parser_middleware`` dispatch if you need first-class ``Content-Type`` support.
