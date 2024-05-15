cmake_minimum_required(VERSION 3.14)

project (emmy_core)

add_library(emmy_core SHARED)

add_dependencies(
    emmy_core
    emmy_debugger
)

set_target_properties(emmy_core PROPERTIES PREFIX "")

target_include_directories(emmy_core
    PRIVATE src
)

target_sources(emmy_core PRIVATE
    src/emmy_core.cpp
)

target_link_libraries(
    emmy_core
    PRIVATE emmy_debugger
)

install(
    TARGETS emmy_core
    LIBRARY DESTINATION bin
    RUNTIME DESTINATION bin
)