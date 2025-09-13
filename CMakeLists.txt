cmake_minimum_required(VERSION 3.10)
project(FoxHttp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Boost REQUIRED COMPONENTS system json)
find_package(spdlog REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_library(foxhttplib STATIC)

add_subdirectory(include)
add_subdirectory(src)

target_link_libraries(foxhttplib ${Boost_LIBRARIES} spdlog::spdlog nlohmann_json::nlohmann_json)