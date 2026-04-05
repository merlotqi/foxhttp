set(FOXHTTP_EXAMPLES
    01_hello_server
    02_routing
    03_middleware_chain
    04_static_files
    05_json_body
    07_graceful_shutdown
    08_error_handling
    09_client_timeout
)

foreach(name ${FOXHTTP_EXAMPLES})
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE foxhttplib)
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  set_target_properties(${name} PROPERTIES FOLDER "examples")
endforeach()

target_compile_definitions(04_static_files PRIVATE
  "FOXHTTP_EXAMPLE_WWW_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/www\""
)
