file(GLOB_RECURSE _headers ${CMAKE_CURRENT_LIST_DIR}/*.hpp  CONFIGURE_DEPEND)

target_sources(foxhttplib PUBLIC ${_headers})
unset(_headers)

target_include_directories(foxhttplib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include/foxhttp>
    ${Boost_INCLUDE_DIRS})