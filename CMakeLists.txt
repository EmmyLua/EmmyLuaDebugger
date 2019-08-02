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
    add_definitions(-DUNICODE -D_UNICODE)
else(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    set(CMAKE_OSX_ARCHITECTURES "i386;x86_64")
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

option(EMMY_BUILD_AS_HOOK "build as hook mode" OFF)
set(EMMY_LUA_VERSION "lua53")

if(${EMMY_LUA_VERSION} STREQUAL "lua53")
    set(EMMY_LUA_DIR "lua-5.3.5")
    add_definitions(-DEMMY_LUA_53)
elseif(${EMMY_LUA_VERSION} STREQUAL "lua52")
    set(EMMY_LUA_DIR "lua-5.2.4")
    add_definitions(-DEMMY_LUA_52)
elseif(${EMMY_LUA_VERSION} STREQUAL "lua51")
    set(EMMY_LUA_DIR "lua-5.1.5")
    add_definitions(-DEMMY_LUA_51)
endif()

add_subdirectory(third-party/${EMMY_LUA_DIR})
add_subdirectory(third-party/libuv-1.29.0)
add_subdirectory(emmy_core)
if(WIN32)
    add_subdirectory(emmy_hook)

    install(
        TARGETS emmy_core ${EMMY_LUA_VERSION}_dll ${EMMY_LUA_VERSION}_exe
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
else(WIN32)
    # install(
    #     TARGETS emmy_core ${EMMY_LUA_VERSION}_dll ${EMMY_LUA_VERSION}_exe
    #     LIBRARY DESTINATION lib
    #     RUNTIME DESTINATION bin
    # )
endif(WIN32)
