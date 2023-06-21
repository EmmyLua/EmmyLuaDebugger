cmake_minimum_required(VERSION 3.14)

project (emmy_tool)

add_executable(emmy_tool)

add_dependencies(
    emmy_tool
    shared
)

target_sources(emmy_tool PRIVATE
    src/utility.h
    src/emmy_tool.h
    src/command_line.h

    src/utility.cpp
    src/emmy_tool.windows.cpp
    src/command_line.cpp
    src/main.cpp
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