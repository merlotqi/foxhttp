cmake_minimum_required(VERSION 3.14...3.24)
project(FoxHttp VERSION 0.1.0 LANGUAGES CXX)

# Always this directory (FoxHttp root), never CMAKE_SOURCE_DIR — that is the top-level
# project when FoxHttp is add_subdirectory()'d as a submodule.
set(FOXHTTP_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(FOXHTTP_BUILD_EXAMPLES "Build examples" OFF)
option(FOXHTTP_BUILD_TESTS "Build unit tests (GoogleTest)" OFF)
option(FOXHTTP_BUILD_BENCHMARKS "Build microbenchmarks (Google Benchmark)" OFF)
option(BUILD_WITH_BROTLI "Build with brotli" OFF)
option(FOXHTTP_ENABLE_TLS "Enable TLS (HTTPS) support" ON)
option(FOXHTTP_ENABLE_JWT "Enable jwt-cpp integration for auth middleware" OFF)
if(BUILD_WITH_BROTLI)
    option(USE_VCPKG_BROTLI "Use vcpkg brotli" OFF)
endif()

find_package(Boost REQUIRED COMPONENTS system json iostreams)
find_package(ZLIB REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
if(FOXHTTP_ENABLE_TLS)
    find_package(OpenSSL REQUIRED)
endif()
if(BUILD_WITH_BROTLI)
    if(USE_VCPKG_BROTLI)
        find_package(unofficial-brotli CONFIG REQUIRED)
    else()
        find_package(Brotli REQUIRED)
    endif()
endif()

if(FOXHTTP_ENABLE_JWT)
    find_package(jwt-cpp CONFIG REQUIRED)
endif()

add_subdirectory(src)

if(FOXHTTP_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(FOXHTTP_BUILD_TESTS OR FOXHTTP_BUILD_BENCHMARKS)
  enable_testing()
endif()

if(FOXHTTP_BUILD_TESTS)
  add_subdirectory(tests)
endif()

if(FOXHTTP_BUILD_BENCHMARKS)
  add_subdirectory(benchmark)
endif()

# Install targets
install(TARGETS foxhttplib
    EXPORT foxhttplib-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include/foxhttp
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

# Generate and install config files
include(CMakePackageConfigHelpers)

# Values substituted into FoxHttpConfig.cmake (how this build was configured)
set(FOXHTTP_CONFIG_ENABLE_TLS ${FOXHTTP_ENABLE_TLS})
set(FOXHTTP_CONFIG_BUILD_WITH_BROTLI ${BUILD_WITH_BROTLI})
if(BUILD_WITH_BROTLI)
  set(FOXHTTP_CONFIG_USE_VCPKG_BROTLI ${USE_VCPKG_BROTLI})
else()
  set(FOXHTTP_CONFIG_USE_VCPKG_BROTLI OFF)
endif()
set(FOXHTTP_CONFIG_ENABLE_JWT ${FOXHTTP_ENABLE_JWT})

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/FoxHttpConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FoxHttpConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/FoxHttpConfig.cmake"
    INSTALL_DESTINATION lib/cmake/FoxHttp
)

# pkg-config (foxhttp.pc.in → foxhttp.pc)
set(FOXHTTP_PC_DESCRIPTION "FoxHttp HTTP server library (Boost.Beast)")
set(FOXHTTP_PC_REQUIRES "spdlog yaml-cpp zlib")
if(FOXHTTP_ENABLE_TLS)
  string(APPEND FOXHTTP_PC_REQUIRES " libssl")
endif()
set(FOXHTTP_PC_LIBS_PRIVATE "-lboost_system -lboost_json -lboost_iostreams")
if(FOXHTTP_ENABLE_JWT)
  string(APPEND FOXHTTP_PC_LIBS_PRIVATE " -ljwt-cpp")
endif()
if(BUILD_WITH_BROTLI)
  string(APPEND FOXHTTP_PC_LIBS_PRIVATE " -lbrotlienc -lbrotlidec -lbrotlicommon")
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/foxhttp.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/foxhttp.pc"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/FoxHttpConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/FoxHttpConfigVersion.cmake"
    DESTINATION lib/cmake/FoxHttp
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/foxhttp.pc"
    DESTINATION lib/pkgconfig
)

install(EXPORT foxhttplib-targets
    FILE FoxHttpTargets.cmake
    NAMESPACE FoxHttp::
    DESTINATION lib/cmake/FoxHttp
)
