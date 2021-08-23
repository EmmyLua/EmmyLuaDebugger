cmake_minimum_required(VERSION 3.11)

project (emmy_tool)

add_executable(emmy_tool)

add_dependencies(
    emmy_tool
    shared
)

target_include_directories(emmy_tool PUBLIC 
    ${emmy_SOURCE_DIR}/include
    ./
)

target_sources(emmy_tool PRIVATE
    ${emmy_SOURCE_DIR}/include/shared/shme.h

    ${emmy_tool_SOURCE_DIR}/src/utility.h
    ${emmy_tool_SOURCE_DIR}/src/emmy_tool.h
    ${emmy_tool_SOURCE_DIR}/src/command_line.h

    ${emmy_tool_SOURCE_DIR}/src/utility.cpp
    ${emmy_tool_SOURCE_DIR}/src/emmy_tool.windows.cpp
    ${emmy_tool_SOURCE_DIR}/src/command_line.cpp
    ${emmy_tool_SOURCE_DIR}/src/main.cpp
)

target_link_libraries(
    emmy_tool
    imagehlp
    ws2_32
    shared
)


install(
    TARGETS emmy_tool
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)