.. _parsers:

Parsers and body parsing
=========================

Parser and ParserFactory
------------------------

Parsers implement ``foxhttp::parser::Parser<T>``:

* ``name()``, ``content_type()``
* ``supports(request)``—whether this parser accepts the request (usually inspects ``Content-Type``).
* ``parse(request)``—returns ``T``.

``foxhttp::parser::ParserFactory::instance()`` holds a registry keyed by ``std::type_index``. Register with ``register_parser<T>(std::shared_ptr<Parser<T>>)``; the headers for ``JsonParser``, ``FormParser``, etc. use a **REGISTER_PARSER** macro to register default implementations at static initialization.

Built-in parsers
----------------

* **JsonParser** → ``nlohmann::json``; configurable limits and validation in ``foxhttp::config::JsonConfig``.
* **FormParser** → ``foxhttp::parser::form_data`` / ``FormData`` (map of field names to ``FormField``); ``application/x-www-form-urlencoded``.
* **MultipartParser** → ``foxhttp::parser::multipart_data`` / ``MultipartData``; ``multipart/form-data`` with boundary extraction.
* **PlainTextParser** → ``std::string``; ``text/*`` and related types per configuration.

BodyParserMiddleware
--------------------

Selects a parser based on the **main** ``Content-Type`` (substring before ``;``, case-insensitive):

* ``application/json`` → ``set_parsed_body<nlohmann::json>(...)``
* ``application/x-www-form-urlencoded`` → stores ``std::shared_ptr<foxhttp::parser::form_data>`` under ``foxhttp::middleware::body_parser_context_keys::form`` (move-only types cannot be stored in ``std::any`` used by ``set_parsed_body``).
* ``multipart/*`` → ``std::shared_ptr<foxhttp::parser::multipart_data>`` under ``foxhttp::middleware::body_parser_context_keys::multipart``.
* ``text/*`` → ``set_parsed_body<std::string>(...)``.

On parse failure, optional **400 Bad Request** response when ``bad_request_on_parse_error`` is true.

Extending
---------

Implement ``foxhttp::parser::Parser<YourType>``, register it in ``ParserFactory``, and optionally extend ``BodyParserMiddleware`` dispatch if you need first-class ``Content-Type`` support.
