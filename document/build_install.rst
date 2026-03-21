.. _build_install:

Build and install
=================

Requirements
------------

* **CMake** 3.16 or newer
* **C++17** compiler
* **Boost** (system, json, iostreams)
* **ZLIB**
* **spdlog**
* **nlohmann_json** (CMake config package)
* **yaml-cpp** (CMake config package)
* **OpenSSL** when ``FOXHTTP_ENABLE_TLS=ON``

Optional:

* **jwt-cpp** when ``FOXHTTP_ENABLE_JWT=ON``
* **Brotli** when ``BUILD_WITH_BROTLI=ON``

Configure and build
-------------------

.. code-block:: bash

   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build

CMake options
-------------

.. list-table::
   :widths: 28 12 60
   :header-rows: 1

   * - Option
     - Default
     - Meaning
   * - ``FOXHTTP_BUILD_EXAMPLES``
     - ``OFF``
     - Build programs under ``examples/``
   * - ``FOXHTTP_BUILD_TESTS``
     - ``OFF``
     - Build GoogleTest-based unit tests under ``tests/`` (GTest via FetchContent)
   * - ``FOXHTTP_ENABLE_TLS``
     - ``ON``
     - HTTPS / WSS; defines ``USING_TLS`` for TLS-only headers
   * - ``FOXHTTP_ENABLE_JWT``
     - ``OFF``
     - JWT-related auth middleware
   * - ``BUILD_WITH_BROTLI``
     - ``OFF``
     - Brotli compression support

Install
-------

.. code-block:: bash

   cmake --install build --prefix /path/to/prefix

This installs the static library ``libfoxhttplib``, public headers, **CMake package** files (``FoxHttpConfig.cmake``, ``FoxHttpTargets.cmake``), and **pkg-config** file ``foxhttp.pc`` under ``lib/pkgconfig/``.

Tests
-----

With ``-DFOXHTTP_BUILD_TESTS=ON``:

.. code-block:: bash

   ctest --test-dir build --output-on-failure

Docker
------

See the repository ``Dockerfile`` for a minimal Ubuntu-based dependency set suitable for CI or local containers.
