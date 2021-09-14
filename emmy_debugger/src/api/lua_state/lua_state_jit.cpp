#include "emmy_debugger/api/lua_state.h"

lua_State* GetMainState_luaJIT(lua_State* L)
{
	return L;
}

std::vector<lua_State*> FindAllCoroutine_luaJIT(lua_State* L)
{
	return std::vector<lua_State*>();
}