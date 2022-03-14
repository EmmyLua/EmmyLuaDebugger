#include "emmy_debugger/lua_version.h"


#if EMMY_LUA_51
LuaVersion luaVersion = LuaVersion::LUA_51;
#elif EMMY_LUA_52
LuaVersion luaVersion = LuaVersion::LUA_52;
#elif EMMY_LUA_53
LuaVersion luaVersion = LuaVersion::LUA_53;
#elif EMMY_LUA_54
LuaVersion luaVersion = LuaVersion::LUA_54;
#elif EMMY_LUA_JIT
LuaVersion luaVersion = LuaVersion::LUA_JIT;
#else
LuaVersion luaVersion = LuaVersion::UNKNOWN;
#endif