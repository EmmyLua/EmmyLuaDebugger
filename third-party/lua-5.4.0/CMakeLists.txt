cmake_minimum_required(VERSION 3.11)

project (lua54)

if(LUA_BUILD_AS_DLL)
    add_library(lua54 SHARED)
else()
    add_library(lua54 STATIC)
endif()


if(WIN32)
    add_definitions( -D_CRT_SECURE_NO_WARNINGS )
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions( -DLUA_USE_LINUX )
    list(APPEND LINK_LIBRARIES readline dl m)
else()
    add_definitions( -DLUA_USE_MACOSX )
    list(APPEND LINK_LIBRARIES readline)
endif()

if(WIN32)
    target_compile_definitions(lua54 PUBLIC -DLUA_BUILD_AS_DLL)
endif(WIN32)

target_sources(lua54 PRIVATE

    #SOURCES
    ${lua54_SOURCE_DIR}/src/lapi.c
    ${lua54_SOURCE_DIR}/src/lauxlib.c
    ${lua54_SOURCE_DIR}/src/lbaselib.c
    ${lua54_SOURCE_DIR}/src/lcode.c
    ${lua54_SOURCE_DIR}/src/lcorolib.c
    ${lua54_SOURCE_DIR}/src/lctype.c
    ${lua54_SOURCE_DIR}/src/ldblib.c
    ${lua54_SOURCE_DIR}/src/ldebug.c
    ${lua54_SOURCE_DIR}/src/ldo.c
    ${lua54_SOURCE_DIR}/src/ldump.c
    ${lua54_SOURCE_DIR}/src/lfunc.c
    ${lua54_SOURCE_DIR}/src/lgc.c
    ${lua54_SOURCE_DIR}/src/linit.c
    ${lua54_SOURCE_DIR}/src/liolib.c
    ${lua54_SOURCE_DIR}/src/llex.c
    ${lua54_SOURCE_DIR}/src/lmathlib.c
    ${lua54_SOURCE_DIR}/src/lmem.c
    ${lua54_SOURCE_DIR}/src/loadlib.c
    ${lua54_SOURCE_DIR}/src/lobject.c
    ${lua54_SOURCE_DIR}/src/lopcodes.c
    ${lua54_SOURCE_DIR}/src/loslib.c
    ${lua54_SOURCE_DIR}/src/lparser.c
    ${lua54_SOURCE_DIR}/src/lstate.c
    ${lua54_SOURCE_DIR}/src/lstring.c
    ${lua54_SOURCE_DIR}/src/lstrlib.c
    ${lua54_SOURCE_DIR}/src/ltable.c
    ${lua54_SOURCE_DIR}/src/ltablib.c
    ${lua54_SOURCE_DIR}/src/ltm.c
    ${lua54_SOURCE_DIR}/src/lundump.c
    ${lua54_SOURCE_DIR}/src/lutf8lib.c
    ${lua54_SOURCE_DIR}/src/lvm.c
    ${lua54_SOURCE_DIR}/src/lzio.c
)
