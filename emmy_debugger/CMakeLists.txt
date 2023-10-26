cmake_minimum_required(VERSION 3.14)

project (emmy_debugger)

add_library(emmy_debugger STATIC)

add_dependencies(
    emmy_debugger
    uv_a
)

target_include_directories(emmy_debugger PUBLIC 
    ${emmy_debugger_SOURCE_DIR}/include
    ${emmy_SOURCE_DIR}/third-party/nlohmann/include
    ${emmy_SOURCE_DIR}/third-party/libuv-1.29.0/include
    PRIVATE src
)

target_sources(emmy_debugger 
    PRIVATE
    #src/api
    src/api/lua_api.cpp
    src/api/lua_state.cpp
    src/api/lua_version.cpp

    #src/transporter
    src/transporter/pipeline_client_transporter.cpp
    src/transporter/pipeline_server_transporter.cpp
    src/transporter/socket_client_transporter.cpp
    src/transporter/socket_server_transporter.cpp
    src/transporter/transporter.cpp

    #src/debugger
    src/debugger/emmy_debugger.cpp
    src/debugger/emmy_debugger_manager.cpp
    src/debugger/emmy_debugger_lib.cpp
    src/debugger/hook_state.cpp 
    src/debugger/extension_point.cpp

    #src/proto
    src/proto/proto.cpp
    src/proto/proto_handler.cpp

    #src/arena
    src/arena/arena.cpp

    #src
    src/emmy_facade.cpp
    src/util.cpp
)


if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(emmy_debugger PUBLIC /utf-8)
endif ()


if(NOT WIN32)
    target_compile_options(emmy_debugger PUBLIC -fPIC)
endif()

if(EMMY_USE_LUA_SOURCE)
    target_compile_definitions(emmy_debugger PUBLIC -DEMMY_USE_LUA_SOURCE)
    target_include_directories(emmy_debugger PUBLIC 
        ${emmy_SOURCE_DIR}/third-party/${EMMY_LUA_DIR}/src
    )
    if(${EMMY_LUA_VERSION} STREQUAL "jit")
        target_link_directories(emmy_debugger PUBLIC 
            ${emmy_SOURCE_DIR}/third-party/${EMMY_LUA_DIR}/src
        )
        target_link_libraries(
            emmy_debugger
            PRIVATE lua51.lib
        )
        target_sources(emmy_debugger
            PRIVATE
            src/api/lua_state/lua_state_jit.cpp
        )
    else()
        add_dependencies(emmy_debugger lua${EMMY_LUA_VERSION})
        target_link_libraries(
            emmy_debugger
            PUBLIC lua${EMMY_LUA_VERSION}
        )
        target_sources(emmy_debugger
            PRIVATE
            src/api/lua_state/lua_state_${EMMY_LUA_VERSION}.cpp
        )
    endif()



elseif(EMMY_COMPILE_AS_LIB)
    message("emmy build as lib")
    target_compile_definitions(emmy_debugger PUBLIC -DEMMY_USE_LUA_SOURCE)
    target_sources(emmy_debugger
        PRIVATE
        src/api/lua_state/lua_state_${EMMY_LUA_VERSION}.cpp
    )
else()
    target_include_directories(emmy_debugger PRIVATE
        ${emmy_SOURCE_DIR}/third-party
    )

    target_sources(emmy_debugger
        PUBLIC
        src/api/lua_api_loader.cpp

        PRIVATE
        src/api/lua_state/lua_state_54.cpp
        src/api/lua_state/lua_state_53.cpp
        src/api/lua_state/lua_state_52.cpp
        src/api/lua_state/lua_state_51.cpp
        src/api/lua_state/lua_state_jit.cpp
    )
   
endif(EMMY_USE_LUA_SOURCE)


target_link_libraries(
    emmy_debugger
    PUBLIC uv_a
)
