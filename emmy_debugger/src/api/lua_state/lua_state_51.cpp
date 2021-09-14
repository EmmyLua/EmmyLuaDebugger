#include "emmy_debugger/api/lua_state.h"
#ifdef EMMY_USE_LUA_SOURCE
#include "lstate.h"
#else
#include "lua-5.1.5/src/lstate.h"
#endif

GCObjectGeneral* GetGCHead_lua51(lua_State* L)
{
	return reinterpret_cast<GCObjectGeneral*>(G(L)->rootgc);
}

lua_State* GetMainState_lua51(lua_State* L)
{
	return  G(L)->mainthread;
}
