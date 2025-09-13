file(GLOB_RECURSE _headers ${CMAKE_CURRENT_LIST_DIR}/*.hpp  CONFIGURE_DEPEND)
file(GLOB_RECURSE _private_headers ${CMAKE_CURRENT_LIST_DIR}/*_p.hpp CONFIGURE_DEPEND)
list(REMOVE_ITEM _headers ${_private_headers})

target_sources(foxhttplib PUBLIC ${_headers})
target_sources(foxhttplib PRIVATE ${_private_headers})
unset(_headers)
unset(_private_headers)

target_include_directories(foxhttplib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include/foxhttp>
    ${Boost_INCLUDE_DIRS})