cmake_minimum_required(VERSION 3.2)

project(EasyHook)
enable_language(ASM_MASM)

#easy_hook
if(WIN32)
    add_definitions( -DEASYHOOK_EXPORTS -DUNICODE -D_UNICODE )
    set(NETFXSDK "C:/Program Files (x86)/Windows Kits/NETFXSDK/4.6.1")
endif()

if( CMAKE_CL_64 )
    set( AUX_ULIB "AUX_ULIB_x64" )
    set( ASM_FILE "HookSpecific_x64" )
else()
    set( AUX_ULIB "AUX_ULIB_x86" )
    set( ASM_FILE "HookSpecific_x86" )
endif()

file(GLOB SRC_LIST
    "EasyHookDll/LocalHook/*.cpp" "EasyHookDll/LocalHook/*.c"
    "EasyHookDll/RemoteHook/*.cpp" "EasyHookDll/RemoteHook/*.c"
    "EasyHookDll/Rtl/*.cpp" "EasyHookDll/Rtl/*.c"

    "DriverShared/Disassembler/libudis86/*.cpp" "DriverShared/Disassembler/libudis86/*.c"
    "DriverShared/LocalHook/*.cpp" "DriverShared/LocalHook/*.c"
    "DriverShared/Rtl/*.cpp" "DriverShared/Rtl/*.c"
    "DriverShared/ASM/${ASM_FILE}.asm"
)

file(GLOB HEADER_LIST
    "DriverShared/*.h"
    "DriverShared/Disassembler/*.h"
    "DriverShared/Disassembler/libudis86/*.h"
    "EasyHookDll/*.h"
)

include_directories(
    DriverShared
    EasyHookDll
    ${NETFXSDK}/Include/um
)

link_directories(
    EasyHookDll
)

source_group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)
source_group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} HEADER_LIST)

add_library (EasyHook SHARED
    EasyHookDll/dllmain.c
    ${HEADER_LIST}
    ${SRC_LIST}
)

set_target_properties(EasyHook PROPERTIES LINK_FLAGS "/SAFESEH:NO")
set_target_properties(EasyHook PROPERTIES COMPILE_FLAGS "/Gz")

target_link_libraries(
    EasyHook
    ${CMAKE_CURRENT_SOURCE_DIR}/EasyHookDll/${AUX_ULIB}.lib
    psapi
)

install(
    TARGETS EasyHook
    LIBRARY DESTINATION bin
    RUNTIME DESTINATION bin
)