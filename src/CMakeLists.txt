file(GLOB_RECURSE _sources ${CMAKE_CURRENT_LIST_DIR}/*.cpp  CONFIGURE_DEPEND)
target_sources(foxhttplib PRIVATE ${_sources})
unset(_sources)