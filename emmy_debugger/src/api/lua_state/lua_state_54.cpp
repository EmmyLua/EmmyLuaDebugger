#include "emmy_debugger/api/lua_state.h"
#include "lua-5.4.0/src/lstate.h"

GCObjectGeneral* GetGCHead_lua54(lua_State* L)
{
	return reinterpret_cast<GCObjectGeneral*>(G(L)->allgc);
}

lua_State* GetMainState_lua54(lua_State* L)
{
	return  G(L)->mainthread;
}