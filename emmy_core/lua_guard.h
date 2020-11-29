#pragma once

#include "api/lua_api.h"

class LuaGlobalIgnoreMetatable
{
public:
	LuaGlobalIgnoreMetatable(lua_State* L);
	~LuaGlobalIgnoreMetatable();
private:
	lua_State* _L;
	int _idx;
	int _metaIdx;
	int _top;
};
