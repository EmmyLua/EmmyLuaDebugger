
set(CMAKE_C_COMPILER zig)
set(CMAKE_C_COMPILER_ARG1 cc)
set(CMAKE_CXX_COMPILER zig)
set(CMAKE_CXX_COMPILER_ARG1 c++)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(ZIG_TARGET_TRIPLE "x86_64-linux-gnu")
    set(ZIG_GLIBC_VERSION "2.17")
    
   
    set(CMAKE_C_FLAGS_INIT "-target ${ZIG_TARGET_TRIPLE} -fPIC -static-libgcc -D_GLIBCXX_USE_CXX11_ABI=0")
    set(CMAKE_CXX_FLAGS_INIT "-target ${ZIG_TARGET_TRIPLE} -fPIC -static-libgcc -static-libstdc++ -D_GLIBCXX_USE_CXX11_ABI=0")
    

    set(CMAKE_EXE_LINKER_FLAGS_INIT "-target ${ZIG_TARGET_TRIPLE} -fuse-ld=lld")
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "-target ${ZIG_TARGET_TRIPLE} -fuse-ld=lld")
    

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --target=${ZIG_TARGET_TRIPLE}.${ZIG_GLIBC_VERSION}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --target=${ZIG_TARGET_TRIPLE}.${ZIG_GLIBC_VERSION}")
    
    message(STATUS "Configured Zig for Linux with glibc ${ZIG_GLIBC_VERSION}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(ZIG_TARGET_TRIPLE "x86_64-windows-gnu")
    set(CMAKE_C_FLAGS_INIT "-target ${ZIG_TARGET_TRIPLE}")
    set(CMAKE_CXX_FLAGS_INIT "-target ${ZIG_TARGET_TRIPLE}")
    
    message(STATUS "Configured Zig for Windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        set(ZIG_TARGET_TRIPLE "aarch64-macos")
    else()
        set(ZIG_TARGET_TRIPLE "x86_64-macos")
    endif()
    
    set(CMAKE_C_FLAGS_INIT "-target ${ZIG_TARGET_TRIPLE}")
    set(CMAKE_CXX_FLAGS_INIT "-target ${ZIG_TARGET_TRIPLE}")
    
    message(STATUS "Configured Zig for macOS (${ZIG_TARGET_TRIPLE})")
endif()

set(CMAKE_C_COMPILER_ID Clang)
set(CMAKE_CXX_COMPILER_ID Clang)

execute_process(
    COMMAND ${CMAKE_C_COMPILER} version
    OUTPUT_VARIABLE ZIG_VERSION_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Using Zig: ${ZIG_VERSION_OUTPUT}")