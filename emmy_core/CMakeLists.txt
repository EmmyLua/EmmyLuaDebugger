cmake_minimum_required(VERSION 3.11)

project (emmy_core)

add_library(emmy_core SHARED)

add_dependencies(
    emmy_core
    emmy_debugger
)

option(EMMY_USE_LUA_SOURCE "Build with lua source" OFF)

set_target_properties(emmy_core PROPERTIES PREFIX "")

target_include_directories(emmy_core PUBLIC 
    ${emmy_SOURCE_DIR}/include
    PRIVATE src
)

target_sources(emmy_core PUBLIC
    ${emmy_SOURCE_DIR}/include/emmy_core/emmy_core.h
    ${emmy_core_SOURCE_DIR}/src/emmy_core.cpp
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