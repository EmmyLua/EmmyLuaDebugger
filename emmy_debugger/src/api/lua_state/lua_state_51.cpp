#include "emmy_debugger/api/lua_state.h"
#include "lua-5.1.5/src/lstate.h"

GCObjectGeneral* GetGCHead_lua51(lua_State* L)
{
	return reinterpret_cast<GCObjectGeneral*>(G(L)->rootgc);
}

lua_State* GetMainState_lua51(lua_State* L)
{
	return  G(L)->mainthread;
}