add_executable(fox 

    main.cpp
    
    api/version.h
    api/version.cpp

)

target_link_libraries(fox foxhttplib gflags::gflags)