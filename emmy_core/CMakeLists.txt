cmake_minimum_required(VERSION 3.2)

project (emmy_core)

add_library(emmy_core SHARED)

option(EMMY_USE_LUA_SOURCE "Build with lua source" OFF)

if (EMMY_USE_LUA_SOURCE)
    if(CMAKE_SYSTEM_NAME MATCHES "Android")
        message("use android lib name")
        set(emmy_core "libemmy_core")
    else(CMAKE_SYSTEM_NAME)
        set(emmy_core "emmy_core.${EMMY_LUA_VERSION}")
    endif()
    target_link_libraries(
        emmy_core
        PUBLIC lua${EMMY_LUA_VERSION}_a
    )
endif()

set_target_properties(emmy_core PROPERTIES PREFIX "")


target_include_directories(emmy_core PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    PRIVATE src
)

target_sources(emmy_core PRIVATE
    ${CMAKE_SOURCE_DIR}/include/emmy_core/emmy_core.h
    ${emmy_core_SOURCE_DIR}/src/emmy_core.cpp
)

add_definitions(-DEMMY_CORE_BUILD_AS_DLL)   

add_dependencies(
    emmy_core
    emmy_debugger
)

target_link_libraries(
    emmy_core
    PUBLIC emmy_debugger
)

install(
    TARGETS emmy_core
    LIBRARY DESTINATION bin
    RUNTIME DESTINATION bin
)