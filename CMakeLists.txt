cmake_minimum_required(VERSION 3.16)
project(FoxHttp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(FOXHTTP_BUILD_EXAMPLES "Build examples" OFF)
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

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/FoxHttpConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/FoxHttpConfigVersion.cmake"
    DESTINATION lib/cmake/FoxHttp
)

install(EXPORT foxhttplib-targets
    NAMESPACE FoxHttp::
    DESTINATION lib/cmake/FoxHttp
)
