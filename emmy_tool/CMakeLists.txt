cmake_minimum_required(VERSION 3.2)

project (emmy_tool)

set(LINK_LIBRARIES uv_a)

file(GLOB SRC_LIST
    *.cpp *.c
)
file(GLOB HEADER_LIST
    *.h
)
file(GLOB SHARED_LIST
    ${CMAKE_SOURCE_DIR}/Shared/*.h
    ${CMAKE_SOURCE_DIR}/Shared/*.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/Shared)

add_executable(emmy_tool ${SRC_LIST} ${HEADER_LIST} ${SHARED_LIST})

target_link_libraries(
    emmy_tool
    imagehlp
)

install(
    TARGETS emmy_tool
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)