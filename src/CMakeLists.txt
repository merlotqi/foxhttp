file(GLOB_RECURSE _sources ${CMAKE_CURRENT_LIST_DIR}/*.cpp CONFIGURE_DEPEND)
file(GLOB_RECURSE _headers ${FOXHTTP_SOURCE_DIR}/include/*.hpp)

add_library(foxhttplib STATIC ${_sources} ${_headers})
add_library(foxhttp::foxhttplib ALIAS foxhttplib)
target_compile_features(foxhttplib PUBLIC cxx_std_20)

# Install headers
install(DIRECTORY ${FOXHTTP_SOURCE_DIR}/include/foxhttp
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

# Set include directories
target_include_directories(foxhttplib
    PUBLIC
        $<BUILD_INTERFACE:${FOXHTTP_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    INTERFACE
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(foxhttplib PUBLIC
    Boost::system
    Boost::json
    Boost::iostreams
    ZLIB::ZLIB
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
)

if(FOXHTTP_ENABLE_TLS)
    target_link_libraries(foxhttplib PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(foxhttplib PRIVATE -DUSING_TLS)
endif()

if(FOXHTTP_ENABLE_JWT)
    target_link_libraries(foxhttplib PUBLIC jwt-cpp::jwt-cpp)
    target_compile_definitions(foxhttplib PRIVATE -DFOXHTTP_ENABLE_JWT)
endif()

if(BUILD_WITH_BROTLI)
    if(USE_VCPKG_BROTLI)
        target_link_libraries(foxhttplib PUBLIC unofficial::brotli::brotlidec unofficial::brotli::brotlienc)
    else()
        target_link_libraries(foxhttplib PUBLIC Brotli::brotlienc Brotli::brotlidec Brotli::brotlicommon)
    endif()
    target_compile_definitions(foxhttplib PRIVATE -DUSING_BROTLI)
endif()

unset(_sources)
unset(_headers)
