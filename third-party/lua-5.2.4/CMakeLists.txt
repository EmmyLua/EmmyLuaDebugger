cmake_minimum_required(VERSION 3.2)
project (lua53)  # project here actually means solution in premake

set(LUA_NAME "lua52")
set(LINK_LIBRARIES ${LUA_NAME}_dll)

if(WIN32)
    add_definitions( -D_CRT_SECURE_NO_WARNINGS )
else()
    add_definitions( -DLUA_USE_MACOSX )
    list(APPEND LINK_LIBRARIES readline)
endif()

file(GLOB HEADER_LIST "src/*.h" "src/lua.hpp")
aux_source_directory(src SRC_LIST)
list(REMOVE_ITEM SRC_LIST "src/lua.c" "src/luac.c")

# 2. lua static library
if(WIN32)
    add_definitions(-DLUA_BUILD_AS_DLL)
endif(WIN32)
add_library (${LUA_NAME}_dll SHARED ${SRC_LIST} ${HEADER_LIST})
set_target_properties(${LUA_NAME}_dll PROPERTIES OUTPUT_NAME "lua")
# 1. lua exe
add_executable(${LUA_NAME}_exe "src/lua.c")
target_link_libraries(${LUA_NAME}_exe ${LINK_LIBRARIES})
link_libraries(${LUA_NAME}_exe ${LUA_NAME}_dll)
set_target_properties(${LUA_NAME}_exe PROPERTIES OUTPUT_NAME "lua")