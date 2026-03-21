if(NOT FOXHTTP_BUILD_BENCHMARKS)
  return()
endif()

message(STATUS "FoxHttp: building microbenchmarks (Google Benchmark via FetchContent)")

include(FetchContent)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  googlebenchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.tar.gz
)
FetchContent_MakeAvailable(googlebenchmark)

file(GLOB_RECURSE FOXHTTP_BENCHMARK_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(BENCH_SOURCE ${FOXHTTP_BENCHMARK_SOURCES})
  get_filename_component(BENCH_NAME ${BENCH_SOURCE} NAME_WE)
  add_executable(${BENCH_NAME} ${BENCH_SOURCE})
  target_link_libraries(${BENCH_NAME} PRIVATE benchmark::benchmark_main foxhttp::foxhttplib)
  target_compile_features(${BENCH_NAME} PRIVATE cxx_std_17)

  add_test(
    NAME ${BENCH_NAME}
    COMMAND ${BENCH_NAME} --benchmark_min_time=0.01s --benchmark_repetitions=1
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  set_tests_properties(${BENCH_NAME} PROPERTIES LABELS "benchmark" TIMEOUT 120)
endforeach()

add_custom_target(
  foxhttp_run_benchmarks
  COMMAND ${CMAKE_CTEST_COMMAND} -L benchmark --output-on-failure
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Run all Google Benchmark targets registered with CTest (label: benchmark)"
)
