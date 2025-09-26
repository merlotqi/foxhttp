cmake_minimum_required(VERSION 3.10)
project(FoxHttp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
option(FOXHTTP_BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_WITH_BROTLI "Build with brotli" OFF)
option(FOXHTTP_ENABLE_TLS "Enable TLS (HTTPS) support" ON)
option(FOXHTTP_ENABLE_JWT "Enable jwt-cpp integration for auth middleware" OFF)
if(BUILD_WITH_BROTLI)
    option(USE_VCPKG_BROTLI "Use vcpkg brotli" OFF)
endif()

find_package(Boost REQUIRED COMPONENTS system json signals2 iostreams)
find_package(ZLIB REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(gflags REQUIRED)
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

add_library(foxhttplib SHARED)

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    target_compile_options(foxhttplib PRIVATE -fvisibility=hidden)
    target_compile_options(foxhttplib PRIVATE -fvisibility-inlines-hidden)
endif()

add_subdirectory(include)
add_subdirectory(src)

target_link_libraries(foxhttplib ${Boost_LIBRARIES} spdlog::spdlog nlohmann_json::nlohmann_json yaml-cpp::yaml-cpp ZLIB::ZLIB)
if(FOXHTTP_ENABLE_TLS)
    target_link_libraries(foxhttplib OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(foxhttplib PRIVATE -DUSING_TLS)
endif()
if(FOXHTTP_ENABLE_JWT)
    target_link_libraries(foxhttplib jwt-cpp::jwt-cpp)
    target_compile_definitions(foxhttplib PRIVATE -DFOXHTTP_ENABLE_JWT)
endif()
message("USE_VCPKG_BROTLI=${USE_VCPKG_BROTLI}")
if(BUILD_WITH_BROTLI)
    if(USE_VCPKG_BROTLI)
        target_link_libraries(foxhttplib unofficial::brotli::brotlidec unofficial::brotli::brotlienc)
    else()
        target_link_libraries(foxhttplib Brotli::brotlienc Brotli::brotlidec Brotli::brotlicommon)
    endif()
endif()
if(BUILD_WITH_BROTLI)
    target_compile_definitions(foxhttplib PRIVATE -DUSING_BROTLI)
endif()

add_subdirectory(fox)

install(TARGETS foxhttplib
    EXPORT foxhttplib-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/foxhttp
    DESTINATION include
)

install(DIRECTORY
    include/foxhttp
    DESTINATION include
)