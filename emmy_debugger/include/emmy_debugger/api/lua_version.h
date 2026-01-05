
#pragma once

// lua version
enum class LuaVersion {
	UNKNOWN,
	LUA_JIT = 50,
	LUA_51 = 51,
	LUA_52 = 52,
	LUA_53 = 53,
	LUA_54 = 54,
	LUA_55 = 55,
};

extern LuaVersion luaVersion;