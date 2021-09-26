cmake_minimum_required(VERSION 3.11)

project (lua51)

if(LUA_BUILD_AS_DLL)
    add_library(lua51 SHARED)
else()
    add_library(lua51 STATIC)
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
    target_compile_definitions(lua51 PUBLIC -DLUA_BUILD_AS_DLL)
endif(WIN32)

target_sources(lua51 PRIVATE

    #SOURCES
    ${lua51_SOURCE_DIR}/src/lapi.c
    ${lua51_SOURCE_DIR}/src/lauxlib.c
    ${lua51_SOURCE_DIR}/src/lbaselib.c
    ${lua51_SOURCE_DIR}/src/lcode.c


    ${lua51_SOURCE_DIR}/src/ldblib.c
    ${lua51_SOURCE_DIR}/src/ldebug.c
    ${lua51_SOURCE_DIR}/src/ldo.c
    ${lua51_SOURCE_DIR}/src/ldump.c
    ${lua51_SOURCE_DIR}/src/lfunc.c
    ${lua51_SOURCE_DIR}/src/lgc.c
    ${lua51_SOURCE_DIR}/src/linit.c
    ${lua51_SOURCE_DIR}/src/liolib.c
    ${lua51_SOURCE_DIR}/src/llex.c
    ${lua51_SOURCE_DIR}/src/lmathlib.c
    ${lua51_SOURCE_DIR}/src/lmem.c
    ${lua51_SOURCE_DIR}/src/loadlib.c
    ${lua51_SOURCE_DIR}/src/lobject.c
    ${lua51_SOURCE_DIR}/src/lopcodes.c
    ${lua51_SOURCE_DIR}/src/loslib.c
    ${lua51_SOURCE_DIR}/src/lparser.c
    ${lua51_SOURCE_DIR}/src/lstate.c
    ${lua51_SOURCE_DIR}/src/lstring.c
    ${lua51_SOURCE_DIR}/src/lstrlib.c
    ${lua51_SOURCE_DIR}/src/ltable.c
    ${lua51_SOURCE_DIR}/src/ltablib.c
    ${lua51_SOURCE_DIR}/src/ltm.c
    ${lua51_SOURCE_DIR}/src/lundump.c

    ${lua51_SOURCE_DIR}/src/lvm.c
    ${lua51_SOURCE_DIR}/src/lzio.c
    ${lua51_SOURCE_DIR}/src/print.c
)
