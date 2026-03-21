.. _packaging:

CMake package and pkg-config
============================

CMake consumers
---------------

After ``cmake --install``, add the prefix to ``CMAKE_PREFIX_PATH`` and use:

.. code-block:: cmake

   find_package(FoxHttp REQUIRED)
   target_link_libraries(myapp PRIVATE FoxHttp::foxhttplib)

``FoxHttpConfig.cmake`` runs ``find_dependency`` for Boost, ZLIB, spdlog, nlohmann_json, yaml-cpp, and optionally OpenSSL, Brotli, jwt-cpp—**matching the options used when FoxHttp was built** (baked into the config file).

pkg-config
----------

``lib/pkgconfig/foxhttp.pc`` is generated at configure time with:

* ``Requires:``—packages such as ``spdlog``, ``yaml-cpp``, ``zlib``, and ``libssl`` when TLS was enabled.
* ``Libs.private:``—Boost and other flags for **static** linking of ``libfoxhttplib.a``; use:

.. code-block:: bash

   pkg-config --static --cflags --libs foxhttp

**nlohmann_json** is header-only and may not appear in ``Requires``; ensure include paths match your install, as you would with the CMake target ``FoxHttp::foxhttplib``.
