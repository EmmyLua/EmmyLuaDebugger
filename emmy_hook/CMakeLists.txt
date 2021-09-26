cmake_minimum_required(VERSION 3.11)

project (emmy_hook)

add_library(emmy_hook SHARED )

set_target_properties(emmy_hook PROPERTIES PREFIX "")

target_include_directories(emmy_hook PUBLIC 
    ${emmy_SOURCE_DIR}/include
    ${emmy_SOURCE_DIR}/third-party/EasyHook/EasyHookDll
    PRIVATE src
)

target_sources(emmy_hook PRIVATE
    ${emmy_SOURCE_DIR}/include/emmy_hook/emmy_hook.h

    #src
    ${emmy_hook_SOURCE_DIR}/src/dllmain.cpp
    ${emmy_hook_SOURCE_DIR}/src/emmy_hook.cpp
    ${emmy_hook_SOURCE_DIR}/src/emmy_hook.windows.cpp
    

    #src/libpe
    ${emmy_hook_SOURCE_DIR}/src/libpe/htable.cpp
    ${emmy_hook_SOURCE_DIR}/src/libpe/htable.h
    ${emmy_hook_SOURCE_DIR}/src/libpe/libpe.cpp
    ${emmy_hook_SOURCE_DIR}/src/libpe/libpe.h
    ${emmy_hook_SOURCE_DIR}/src/libpe/llist.cpp
    ${emmy_hook_SOURCE_DIR}/src/libpe/llist.h

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