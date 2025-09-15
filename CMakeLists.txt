cmake_minimum_required(VERSION 3.10)
project(FoxHttp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
option(FOXHTTP_BUILD_EXAMPLES "Build examples" OFF)


find_package(Boost REQUIRED COMPONENTS system json signals2)
find_package(spdlog REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(gflags REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_library(foxhttplib SHARED)

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    target_compile_options(MyLibrary PRIVATE -fvisibility=hidden)
    target_compile_options(MyLibrary PRIVATE -fvisibility-inlines-hidden)
endif()

add_subdirectory(include)
add_subdirectory(src)

target_link_libraries(foxhttplib ${Boost_LIBRARIES} spdlog::spdlog nlohmann_json::nlohmann_json yaml-cpp::yaml-cpp)

if(FOXHTTP_BUILD_EXAMPLES)
    add_subdirectory(examples)
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