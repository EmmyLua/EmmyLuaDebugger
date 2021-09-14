#include "emmy_debugger/api/lua_state.h"
#include "emmy_debugger/lua_version.h"

#ifdef EMMY_USE_LUA_SOURCE
#if defined(EMMY_LUA_51)
#define LuaSwitchDo(__Lua51__, __Lua52__ , __Lua53__, __Lua54__) return __Lua51__
#elif defined(EMMY_LUA_52)
#define LuaSwitchDo(__Lua51__, __Lua52__ , __Lua53__, __Lua54__) return __Lua52__
#elif defined(EMMY_LUA_53)
#define LuaSwitchDo(__Lua51__, __Lua52__ , __Lua53__, __Lua54__) return __Lua53__
#elif defined(EMMY_LUA_54)
#define LuaSwitchDo(__Lua51__, __Lua52__ , __Lua53__, __Lua54__) return __Lua54__
#endif
#else
#define LuaSwitchDo(__Lua51__, __Lua52__ , __Lua53__, __Lua54__)\
switch(luaVersion){ \
	case LuaVersion::LUA_51:\
	{  \
		return __Lua51__;\
	}\
	case LuaVersion::LUA_52:\
	{\
		return __Lua52__;\
	}\
	case LuaVersion::LUA_53:\
	{\
		return __Lua53__;\
	}\
	case LuaVersion::LUA_54:\
	{\
		return __Lua54__;\
	}\
	default:return nullptr;\
}
#endif


GCObjectGeneral* GetGCHead(lua_State* L)
{
	LuaSwitchDo(
		GetGCHead_lua51(L),
		GetGCHead_lua52(L),
		GetGCHead_lua53(L),
		GetGCHead_lua54(L)
	);
}


lua_State* GetMainState(lua_State* L)
{
	LuaSwitchDo(
		GetMainState_lua51(L),
		GetMainState_lua52(L),
		GetMainState_lua53(L),
		GetMainState_lua54(L)
	);
}
