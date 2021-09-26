#include "emmy_debugger/api/lua_state.h"
#ifdef EMMY_USE_LUA_SOURCE
#include "lstate.h"
#else
#include "lua-5.3.5/src/lstate.h"
#endif

lua_State* GetMainState_lua53(lua_State* L)
{
	return G(L)->mainthread;
}


std::vector<lua_State*> FindAllCoroutine_lua53(lua_State* L)
{
	std::vector<lua_State*> result;
	auto head = G(L)->allgc;

	while (head)
	{
		if (head->tt == LUA_TTHREAD)
		{
			result.push_back(reinterpret_cast<lua_State*>(head));
		}
		head = head->next;
	}

	return result;
}