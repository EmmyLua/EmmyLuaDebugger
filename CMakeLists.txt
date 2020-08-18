cmake_minimum_required(VERSION 3.2)

project (emmy)
set(CMAKE_INSTALL_PREFIX install)

if (MSVC)
    set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
    )
    foreach(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
    # add_definitions(-DUNICODE -D_UNICODE)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
elseif(CMAKE_SYSTEM_NAME MATCHES "Android")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
    # set(CMAKE_OSX_ARCHITECTURES "i386;x86_64")
endif(MSVC)

macro(source_group_by_dir proj_dir source_files)
    if(MSVC OR APPLE)
        get_filename_component(sgbd_cur_dir ${proj_dir} ABSOLUTE)
        foreach(sgbd_file ${${source_files}})
            get_filename_component(sgbd_abs_file ${sgbd_file} ABSOLUTE)
            file(RELATIVE_PATH sgbd_fpath ${sgbd_cur_dir} ${sgbd_abs_file})
            string(REGEX REPLACE "\(.*\)/.*" \\1 sgbd_group_name ${sgbd_fpath})
            string(COMPARE EQUAL ${sgbd_fpath} ${sgbd_group_name} sgbd_nogroup)
            string(REPLACE "/" "\\" sgbd_group_name ${sgbd_group_name})
            if(sgbd_nogroup)
                set(sgbd_group_name "\\")
            endif(sgbd_nogroup)
            source_group(${sgbd_group_name} FILES ${sgbd_file})
        endforeach(sgbd_file)
    endif(MSVC OR APPLE)
endmacro(source_group_by_dir)

set(EMMY_LUA_VERSION "53" CACHE STRING "Lua version: 51/52/53")

if(${EMMY_LUA_VERSION} STREQUAL "53")
    set(EMMY_LUA_DIR "lua-5.3.5")
    add_definitions(-DEMMY_LUA_53)
elseif(${EMMY_LUA_VERSION} STREQUAL "52")
    set(EMMY_LUA_DIR "lua-5.2.4")
    add_definitions(-DEMMY_LUA_52)
elseif(${EMMY_LUA_VERSION} STREQUAL "51")
    set(EMMY_LUA_DIR "lua-5.1.5")
    add_definitions(-DEMMY_LUA_51)
endif()

add_subdirectory(third-party/${EMMY_LUA_DIR})
add_subdirectory(third-party/libuv-1.29.0)
add_subdirectory(emmy_core)
if(WIN32)
    add_subdirectory(emmy_tool)
    add_subdirectory(third-party/EasyHook)
endif(WIN32)
