cmake_minimum_required(VERSION 3.11)

project (emmy_debugger)

add_library(emmy_debugger STATIC )

add_dependencies(
    emmy_debugger
    uv
)

target_include_directories(emmy_debugger PUBLIC 
    ${emmy_SOURCE_DIR}/include
    ${emmy_SOURCE_DIR}/third-party/rapidjson-1.1.0/include
    ${emmy_SOURCE_DIR}/third-party/libuv-1.29.0/include
    PRIVATE src
)

target_sources(emmy_debugger PUBLIC

    ${emmy_SOURCE_DIR}/include/emmy_debugger/emmy_debugger.h
    ${emmy_SOURCE_DIR}/include/emmy_debugger/emmy_debugger_manager.h
    ${emmy_SOURCE_DIR}/include/emmy_debugger/emmy_facade.h
    ${emmy_SOURCE_DIR}/include/emmy_debugger/hook_state.h
    ${emmy_SOURCE_DIR}/include/emmy_debugger/lua_version.h
    ${emmy_SOURCE_DIR}/include/emmy_debugger/transporter.h
    ${emmy_SOURCE_DIR}/include/emmy_debugger/types.h
    ${emmy_SOURCE_DIR}/include/emmy_debugger/emmy_helper.h
    
    ${emmy_SOURCE_DIR}/include/emmy_debugger/api/lua_api.h

    ${emmy_SOURCE_DIR}/include/emmy_debugger/proto/pipeline_client_transporter.h
    ${emmy_SOURCE_DIR}/include/emmy_debugger/proto/pipeline_server_transporter.h
    ${emmy_SOURCE_DIR}/include/emmy_debugger/proto/socket_client_transporter.h
    ${emmy_SOURCE_DIR}/include/emmy_debugger/proto/socket_server_transporter.h

    #src/api
    ${emmy_debugger_SOURCE_DIR}/src/api/lua_api.cpp
    ${emmy_debugger_SOURCE_DIR}/src/api/lua_state.cpp

    #src/proto
    ${emmy_debugger_SOURCE_DIR}/src/proto/pipeline_client_transporter.cpp
    ${emmy_debugger_SOURCE_DIR}/src/proto/pipeline_server_transporter.cpp
    ${emmy_debugger_SOURCE_DIR}/src/proto/socket_client_transporter.cpp
    ${emmy_debugger_SOURCE_DIR}/src/proto/socket_server_transporter.cpp

    #src
    ${emmy_debugger_SOURCE_DIR}/src/emmy_debugger.cpp
    ${emmy_debugger_SOURCE_DIR}/src/emmy_debugger_manager.cpp
    ${emmy_debugger_SOURCE_DIR}/src/emmy_facade.cpp
    ${emmy_debugger_SOURCE_DIR}/src/emmy_helper.cpp
    ${emmy_debugger_SOURCE_DIR}/src/hook_state.cpp
    ${emmy_debugger_SOURCE_DIR}/src/stack.cpp
    ${emmy_debugger_SOURCE_DIR}/src/transporter.cpp
    ${emmy_debugger_SOURCE_DIR}/src/lua_version.cpp
)

target_compile_definitions(emmy_debugger PUBLIC -DRAPIDJSON_HAS_STDSTRING)

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
            ${emmy_debugger_SOURCE_DIR}/src/api/lua_state/lua_state_jit.cpp
        )
    else()
        target_link_libraries(
            emmy_debugger
            PRIVATE lua${EMMY_LUA_VERSION}
        )
        target_sources(emmy_debugger
            PRIVATE
            ${emmy_debugger_SOURCE_DIR}/src/api/lua_state/lua_state_${EMMY_LUA_VERSION}.cpp
        )
    endif()



elseif(EMMY_COMPILE_AS_LIB)
    message("emmy build as lib")
    target_compile_definitions(emmy_debugger PUBLIC -DEMMY_USE_LUA_SOURCE)
    target_sources(emmy_debugger
        PRIVATE
        ${emmy_debugger_SOURCE_DIR}/src/api/lua_state/lua_state_${EMMY_LUA_VERSION}.cpp
    )
else()
    target_include_directories(emmy_debugger PRIVATE
        ${emmy_SOURCE_DIR}/third-party
    )

    target_sources(emmy_debugger
        PUBLIC
        ${emmy_SOURCE_DIR}/include/emmy_debugger/api/lua_api_loader.h
        ${emmy_debugger_SOURCE_DIR}/src/api/lua_api_loader.cpp

        PRIVATE
        ${emmy_debugger_SOURCE_DIR}/src/api/lua_state/lua_state_54.cpp
        ${emmy_debugger_SOURCE_DIR}/src/api/lua_state/lua_state_53.cpp
        ${emmy_debugger_SOURCE_DIR}/src/api/lua_state/lua_state_52.cpp
        ${emmy_debugger_SOURCE_DIR}/src/api/lua_state/lua_state_51.cpp
        ${emmy_debugger_SOURCE_DIR}/src/api/lua_state/lua_state_jit.cpp
    )
   


endif(EMMY_USE_LUA_SOURCE)


target_link_libraries(
    emmy_debugger
    PUBLIC uv_a
)
