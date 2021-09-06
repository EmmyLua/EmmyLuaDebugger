cmake_minimum_required(VERSION 3.11)

project (lua53)

if(LUA_BUILD_AS_DLL)
    add_library(lua53 SHARED)
else()
    add_library(lua53 STATIC)
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
    target_compile_definitions(lua53 PUBLIC -DLUA_BUILD_AS_DLL)
endif(WIN32)

target_sources(lua53 PRIVATE

    #SOURCES
    ${lua53_SOURCE_DIR}/src/lapi.c
    ${lua53_SOURCE_DIR}/src/lauxlib.c
    ${lua53_SOURCE_DIR}/src/lbaselib.c
    ${lua53_SOURCE_DIR}/src/lcode.c
    ${lua53_SOURCE_DIR}/src/lcorolib.c
    ${lua53_SOURCE_DIR}/src/lctype.c
    ${lua53_SOURCE_DIR}/src/ldblib.c

    ${lua53_SOURCE_DIR}/src/lbitlib.c
    
    ${lua53_SOURCE_DIR}/src/ldebug.c
    ${lua53_SOURCE_DIR}/src/ldo.c
    ${lua53_SOURCE_DIR}/src/ldump.c
    ${lua53_SOURCE_DIR}/src/lfunc.c
    ${lua53_SOURCE_DIR}/src/lgc.c
    ${lua53_SOURCE_DIR}/src/linit.c
    ${lua53_SOURCE_DIR}/src/liolib.c
    ${lua53_SOURCE_DIR}/src/llex.c
    ${lua53_SOURCE_DIR}/src/lmathlib.c
    ${lua53_SOURCE_DIR}/src/lmem.c
    ${lua53_SOURCE_DIR}/src/loadlib.c
    ${lua53_SOURCE_DIR}/src/lobject.c
    ${lua53_SOURCE_DIR}/src/lopcodes.c
    ${lua53_SOURCE_DIR}/src/loslib.c
    ${lua53_SOURCE_DIR}/src/lparser.c
    ${lua53_SOURCE_DIR}/src/lstate.c
    ${lua53_SOURCE_DIR}/src/lstring.c
    ${lua53_SOURCE_DIR}/src/lstrlib.c
    ${lua53_SOURCE_DIR}/src/ltable.c
    ${lua53_SOURCE_DIR}/src/ltablib.c
    ${lua53_SOURCE_DIR}/src/ltm.c
    ${lua53_SOURCE_DIR}/src/lundump.c
    ${lua53_SOURCE_DIR}/src/lutf8lib.c
    ${lua53_SOURCE_DIR}/src/lvm.c
    ${lua53_SOURCE_DIR}/src/lzio.c
)
