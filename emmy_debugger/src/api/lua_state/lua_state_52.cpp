#include "emmy_debugger/api/lua_state.h"
#include "lua-5.2.4/src/lstate.h"

GCObjectGeneral* GetGCHead_lua52(lua_State* L)
{
	return reinterpret_cast<GCObjectGeneral*>(G(L)->allgc);
}

lua_State* GetMainState_lua52(lua_State* L)
{
	return G(L)->mainthread;
}

bool isGlobalStateReady_lua52(lua_State* L)
{
	return G(L) != nullptr;
}