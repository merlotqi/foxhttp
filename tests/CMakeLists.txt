if(NOT FOXHTTP_BUILD_TESTS)
  return()
endif()

message(STATUS "FoxHttp: building unit tests (GoogleTest)")

include(FetchContent)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

file(GLOB_RECURSE FOXHTTP_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(TEST_SOURCE ${FOXHTTP_TEST_SOURCES})
  get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
  add_executable(${TEST_NAME} ${TEST_SOURCE})
  target_link_libraries(${TEST_NAME} PRIVATE GTest::gtest_main foxhttp::foxhttplib)
  target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
  gtest_discover_tests(
    ${TEST_NAME}
    TEST_PREFIX "${TEST_NAME}."
    PROPERTIES DISCOVERY_TIMEOUT 20
  )
endforeach()

add_custom_target(
  foxhttp_run_tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Run FoxHttp unit tests (ctest)"
)
