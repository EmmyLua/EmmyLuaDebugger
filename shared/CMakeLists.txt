cmake_minimum_required(VERSION 3.14)

project (shared)

add_library(shared STATIC)

target_include_directories(shared PUBLIC 
    ${shared_SOURCE_DIR}/include
)

target_sources(shared PRIVATE
    src/shme.cpp
)

target_link_libraries(
    shared
)
