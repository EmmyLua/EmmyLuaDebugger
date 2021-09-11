#include "emmy_debugger/api/lua_state.h"
#include "lua-5.3.5/src/lstate.h"

GCObjectGeneral* GetGCHead_lua53(lua_State* L)
{
	return reinterpret_cast<GCObjectGeneral*>(G(L)->allgc);
}

lua_State* GetMainState_lua53(lua_State* L)
{
	return G(L)->mainthread;
}
