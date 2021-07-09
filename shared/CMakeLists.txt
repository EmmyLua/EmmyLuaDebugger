cmake_minimum_required(VERSION 3.2)

project (shared)

add_library(shared STATIC)

target_include_directories(shared PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ./
)

target_sources(shared PRIVATE
    ${CMAKE_SOURCE_DIR}/include/shared/shme.h

    ${shared_SOURCE_DIR}/src/shme.cpp
)

target_link_libraries(
    shared
)
