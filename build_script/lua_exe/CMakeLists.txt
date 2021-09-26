cmake_minimum_required(VERSION 3.11)

project (lua)

add_executable(lua)

set(EMMY_LUA_VERSION "54" CACHE STRING "Lua version: 51/52/53/54")

if(${EMMY_LUA_VERSION} STREQUAL "54")
    set(EMMY_LUA_DIR "lua-5.4.0")
elseif(${EMMY_LUA_VERSION} STREQUAL "53")
    set(EMMY_LUA_DIR "lua-5.3.5")
elseif(${EMMY_LUA_VERSION} STREQUAL "52")
    set(EMMY_LUA_DIR "lua-5.2.4")
elseif(${EMMY_LUA_VERSION} STREQUAL "51")
    set(EMMY_LUA_DIR "lua-5.1.5")
endif()

option(LUA_BUILD_AS_DLL "USE" ON)
set(lua_dll_SOURCE_DIR "../../third-party/${EMMY_LUA_DIR}")

add_subdirectory("${lua_dll_SOURCE_DIR}" luadll.out)

add_dependencies(lua "lua${EMMY_LUA_VERSION}")

target_sources(lua PUBLIC

    #SOURCES
    ${lua_dll_SOURCE_DIR}/src/lua.c
)

target_link_libraries(lua "lua${EMMY_LUA_VERSION}")


install(
    TARGETS lua
    LIBRARY DESTINATION ./
    RUNTIME DESTINATION ./
)

install(
    TARGETS "lua${EMMY_LUA_VERSION}"
    LIBRARY DESTINATION ./
    RUNTIME DESTINATION ./
)