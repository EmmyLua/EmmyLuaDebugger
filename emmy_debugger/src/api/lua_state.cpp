#include "emmy_debugger/api/lua_state.h"
#include "emmy_debugger/lua_version.h"

GCObjectGeneral* GetGCHead(lua_State* L)
{
	switch (luaVersion)
	{
	case LuaVersion::LUA_51:
		{
			return GetGCHead_lua51(L);
		}
	case LuaVersion::LUA_52:
		{
			return GetGCHead_lua52(L);
		}
	case LuaVersion::LUA_53:
		{
			return GetGCHead_lua53(L);
		}
	case LuaVersion::LUA_54:
		{
			return GetGCHead_lua54(L);
		}
	}

	return nullptr;
}


lua_State* GetMainState(lua_State* L)
{
	switch (luaVersion)
	{
	case LuaVersion::LUA_51:
		{
			return GetMainState_lua51(L);
		}
	case LuaVersion::LUA_52:
		{
			return GetMainState_lua52(L);
		}
	case LuaVersion::LUA_53:
		{
			return GetMainState_lua53(L);
		}
	case LuaVersion::LUA_54:
		{
			return GetMainState_lua54(L);
		}
	}

	return nullptr;
}


bool isGlobalStateReady(lua_State* L) 
{
	switch (luaVersion)
	{
	case LuaVersion::LUA_51:
	{
		return isGlobalStateReady_lua51(L);
	}
	case LuaVersion::LUA_52:
	{
		return isGlobalStateReady_lua52(L);
	}
	case LuaVersion::LUA_53:
	{
		return isGlobalStateReady_lua53(L);
	}
	case LuaVersion::LUA_54:
	{
		return isGlobalStateReady_lua54(L);
	}
	}

	return nullptr;
}