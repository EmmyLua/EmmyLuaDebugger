cmake_minimum_required(VERSION 3.11)

project (lua52)

if(LUA_BUILD_AS_DLL)
    add_library(lua52 SHARED)
else()
    add_library(lua52 STATIC)
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
    target_compile_definitions(lua52 PUBLIC -DLUA_BUILD_AS_DLL)
endif(WIN32)

target_sources(lua52 PRIVATE

    #SOURCES
    ${lua52_SOURCE_DIR}/src/lapi.c
    ${lua52_SOURCE_DIR}/src/lauxlib.c
    ${lua52_SOURCE_DIR}/src/lbaselib.c
    ${lua52_SOURCE_DIR}/src/lcode.c
    ${lua52_SOURCE_DIR}/src/lcorolib.c
    ${lua52_SOURCE_DIR}/src/lctype.c
    ${lua52_SOURCE_DIR}/src/ldblib.c

    ${lua52_SOURCE_DIR}/src/lbitlib.c

    ${lua52_SOURCE_DIR}/src/ldebug.c
    ${lua52_SOURCE_DIR}/src/ldo.c
    ${lua52_SOURCE_DIR}/src/ldump.c
    ${lua52_SOURCE_DIR}/src/lfunc.c
    ${lua52_SOURCE_DIR}/src/lgc.c
    ${lua52_SOURCE_DIR}/src/linit.c
    ${lua52_SOURCE_DIR}/src/liolib.c
    ${lua52_SOURCE_DIR}/src/llex.c
    ${lua52_SOURCE_DIR}/src/lmathlib.c
    ${lua52_SOURCE_DIR}/src/lmem.c
    ${lua52_SOURCE_DIR}/src/loadlib.c
    ${lua52_SOURCE_DIR}/src/lobject.c
    ${lua52_SOURCE_DIR}/src/lopcodes.c
    ${lua52_SOURCE_DIR}/src/loslib.c
    ${lua52_SOURCE_DIR}/src/lparser.c
    ${lua52_SOURCE_DIR}/src/lstate.c
    ${lua52_SOURCE_DIR}/src/lstring.c
    ${lua52_SOURCE_DIR}/src/lstrlib.c
    ${lua52_SOURCE_DIR}/src/ltable.c
    ${lua52_SOURCE_DIR}/src/ltablib.c
    ${lua52_SOURCE_DIR}/src/ltm.c
    ${lua52_SOURCE_DIR}/src/lundump.c

    ${lua52_SOURCE_DIR}/src/lvm.c
    ${lua52_SOURCE_DIR}/src/lzio.c
)
