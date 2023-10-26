cmake_minimum_required(VERSION 3.11)

project (emmy_hook)

add_library(emmy_hook SHARED)

set_target_properties(emmy_hook PROPERTIES PREFIX "")

target_include_directories(emmy_hook PUBLIC 
    ${emmy_SOURCE_DIR}/include
    ${emmy_SOURCE_DIR}/third-party/EasyHook/EasyHookDll
    PRIVATE src
)

target_sources(emmy_hook PRIVATE
    #src
    src/dllmain.cpp
    src/emmy_hook.cpp
    src/emmy_hook.windows.cpp
    

    #src/libpe
    src/libpe/htable.cpp
    src/libpe/htable.h
    src/libpe/libpe.cpp
    src/libpe/libpe.h
    src/libpe/llist.cpp
    src/libpe/llist.h

)

add_dependencies(
    emmy_hook
    emmy_debugger
    EasyHook
    shared
)

target_link_libraries(
    emmy_hook
    PRIVATE 
    emmy_debugger
    EasyHook
    shared
)

install(
    TARGETS emmy_hook
    LIBRARY DESTINATION bin
    RUNTIME DESTINATION bin
)