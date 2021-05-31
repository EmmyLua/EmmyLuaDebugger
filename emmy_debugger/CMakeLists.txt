cmake_minimum_required(VERSION 3.2)

project (emmy_debugger)

add_library(emmy_debugger STATIC )

add_dependencies(
    emmy_debugger
    uv
)

target_include_directories(emmy_debugger PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third-party/rapidjson-1.1.0/include
    ${CMAKE_SOURCE_DIR}/third-party/libuv-1.29.0/include
    PRIVATE src
)

target_sources(emmy_debugger PRIVATE

    ${CMAKE_SOURCE_DIR}/include/emmy_debugger/emmy_debugger.h
    ${CMAKE_SOURCE_DIR}/include/emmy_debugger/emmy_facade.h
    ${CMAKE_SOURCE_DIR}/include/emmy_debugger/hook_state.h
    ${CMAKE_SOURCE_DIR}/include/emmy_debugger/lua_version.h
    ${CMAKE_SOURCE_DIR}/include/emmy_debugger/transporter.h
    ${CMAKE_SOURCE_DIR}/include/emmy_debugger/types.h
    ${CMAKE_SOURCE_DIR}/include/emmy_debugger/emmy_helper.h
    
    ${CMAKE_SOURCE_DIR}/include/emmy_debugger/api/lua_api.h

    ${CMAKE_SOURCE_DIR}/include/emmy_debugger/proto/pipeline_client_transporter.h
    ${CMAKE_SOURCE_DIR}/include/emmy_debugger/proto/pipeline_server_transporter.h
    ${CMAKE_SOURCE_DIR}/include/emmy_debugger/proto/socket_client_transporter.h
    ${CMAKE_SOURCE_DIR}/include/emmy_debugger/proto/socket_server_transporter.h

    #src/api
    ${emmy_debugger_SOURCE_DIR}/src/api/lua_api.cpp

    #src/proto
    ${emmy_debugger_SOURCE_DIR}/src/proto/pipeline_client_transporter.cpp
    ${emmy_debugger_SOURCE_DIR}/src/proto/pipeline_server_transporter.cpp
    ${emmy_debugger_SOURCE_DIR}/src/proto/socket_client_transporter.cpp
    ${emmy_debugger_SOURCE_DIR}/src/proto/socket_server_transporter.cpp

    #src
    ${emmy_debugger_SOURCE_DIR}/src/emmy_debugger.cpp
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
        ${CMAKE_SOURCE_DIR}/third-party/${EMMY_LUA_DIR}/src
    )
    list(APPEND LINK_LIBRARIES lua${EMMY_LUA_VERSION}_a)
    target_link_libraries(
        emmy_debugger
        PUBLIC lua${EMMY_LUA_VERSION}_a
    )
else()
    target_sources(emmy_debugger
        PUBLIC
        ${CMAKE_SOURCE_DIR}/include/emmy_debugger/api/lua_api_loader.h
        ${emmy_debugger_SOURCE_DIR}/src/api/lua_api_loader.cpp
    )

endif(EMMY_USE_LUA_SOURCE)


target_link_libraries(
    emmy_debugger
    PUBLIC uv_a
)
