cmake_minimum_required(VERSION 3.2)

project (emmy_core)

option(EMMY_CORE_BUILD_AS_DLL "Build as dll." ON)
option(EMMY_USE_LUA_SOURCE "Build with lua source" OFF)

set(LINK_LIBRARIES uv_a)

file(GLOB SRC_LIST
    *.cpp *.c proto/*.cpp
)
file(GLOB HEADER_LIST
    *.h proto/*.h
)

# rapid json
add_definitions(-DRAPIDJSON_HAS_STDSTRING)

if(EMMY_CORE_BUILD_AS_DLL)
    add_definitions(-DEMMY_CORE_BUILD_AS_DLL)    
endif(EMMY_CORE_BUILD_AS_DLL)

if(EMMY_BUILD_AS_HOOK)
    add_definitions(-DEMMY_BUILD_AS_HOOK)
endif(EMMY_BUILD_AS_HOOK)

if(EMMY_USE_LUA_SOURCE)
    add_definitions(-DEMMY_USE_LUA_SOURCE)
    include_directories(
        ${CMAKE_SOURCE_DIR}/third-party/${EMMY_LUA_DIR}/src
    )
    list(APPEND LINK_LIBRARIES ${EMMY_LUA_VERSION}_dll)
else(EMMY_USE_LUA_SOURCE)
    file(GLOB SRC_LIST
        *.cpp *.c api/*.cpp proto/*.cpp
    )
    file(GLOB HEADER_LIST
        *.h api/*.h proto/*.h
    )
endif(EMMY_USE_LUA_SOURCE)

include_directories(
    ${CMAKE_SOURCE_DIR}/third-party/libuv-1.29.0/include
    ${CMAKE_SOURCE_DIR}/third-party/rapidjson-1.1.0/include
)

add_library(emmy_core SHARED ${SRC_LIST} ${HEADER_LIST})
set_target_properties(emmy_core PROPERTIES PREFIX "")
if(WIN32)
    add_dependencies(emmy_core easy_hook uv_a)
    target_link_libraries(emmy_core easy_hook uv_a)
else(WIN32)
    add_dependencies(emmy_core ${LINK_LIBRARIES})
    target_link_libraries(emmy_core ${LINK_LIBRARIES})
endif(WIN32)

install(
    TARGETS emmy_core
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)