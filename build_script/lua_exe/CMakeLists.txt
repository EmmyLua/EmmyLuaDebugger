cmake_minimum_required(VERSION 3.11)

project (lua)

add_executable(lua)

set(EMMY_LUA_VERSION "55" CACHE STRING "Lua version: 51/52/53/54/55")

if(${EMMY_LUA_VERSION} STREQUAL "55")
    set(EMMY_LUA_DIR "lua-5.5.0")
elseif(${EMMY_LUA_VERSION} STREQUAL "54")
    set(EMMY_LUA_DIR "lua-5.4.6")
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

# Add luac executable
add_executable(luac)

target_sources(luac PRIVATE
    ${lua_dll_SOURCE_DIR}/src/luac.c
    # Lua core library sources (required for luac)
    ${lua_dll_SOURCE_DIR}/src/lapi.c
    ${lua_dll_SOURCE_DIR}/src/lcode.c
    ${lua_dll_SOURCE_DIR}/src/lctype.c
    ${lua_dll_SOURCE_DIR}/src/ldebug.c
    ${lua_dll_SOURCE_DIR}/src/ldo.c
    ${lua_dll_SOURCE_DIR}/src/ldump.c
    ${lua_dll_SOURCE_DIR}/src/lfunc.c
    ${lua_dll_SOURCE_DIR}/src/lgc.c
    ${lua_dll_SOURCE_DIR}/src/llex.c
    ${lua_dll_SOURCE_DIR}/src/lmem.c
    ${lua_dll_SOURCE_DIR}/src/lobject.c
    ${lua_dll_SOURCE_DIR}/src/lopcodes.c
    ${lua_dll_SOURCE_DIR}/src/lparser.c
    ${lua_dll_SOURCE_DIR}/src/lstate.c
    ${lua_dll_SOURCE_DIR}/src/lstring.c
    ${lua_dll_SOURCE_DIR}/src/ltable.c
    ${lua_dll_SOURCE_DIR}/src/ltm.c
    ${lua_dll_SOURCE_DIR}/src/lundump.c
    ${lua_dll_SOURCE_DIR}/src/lvm.c
    ${lua_dll_SOURCE_DIR}/src/lzio.c
    ${lua_dll_SOURCE_DIR}/src/lauxlib.c
)

if(WIN32)
    target_compile_definitions(luac PRIVATE _CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_compile_definitions(luac PRIVATE LUA_USE_LINUX)
    target_link_libraries(luac m dl)
else()
    target_compile_definitions(luac PRIVATE LUA_USE_MACOSX)
    target_link_libraries(luac m)
endif()


install(
    TARGETS lua luac
    LIBRARY DESTINATION ./
    RUNTIME DESTINATION ./
)

install(
    TARGETS "lua${EMMY_LUA_VERSION}"
    LIBRARY DESTINATION ./
    RUNTIME DESTINATION ./
)