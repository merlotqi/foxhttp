.. _config:

Configuration
=============

configs.hpp
-----------

``configs.hpp`` defines **defaulted structs** used across the library:

* ``json_config``, ``form_config``, ``multipart_config``, ``plain_text_config``—parser limits and behavior.
* ``session_limits``—timeouts, max header/body sizes, **keep-alive** (``enable_keep_alive``, ``max_requests_per_connection``), and related session knobs.
* ``strand_pool_config``, ``timer_manager_config``, and other tuning structures.

These are **not** a remote configuration service; they are plain C++ types you construct and pass where the API requires them.

config_manager and YAML
-----------------------

When present in the build, **config_manager** and related headers integrate **yaml-cpp** for loading YAML configuration. If you build a minimal tree without them, use only ``configs.hpp`` and construct limits in code.

TLS
---

When ``USING_TLS`` is defined, ``config/tls.hpp`` and ``ssl_server`` / ``ssl_session`` use OpenSSL-backed TLS settings (certificate paths, cipher preferences—see headers and examples).
