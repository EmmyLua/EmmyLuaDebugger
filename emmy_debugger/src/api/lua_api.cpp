/*
* Copyright (c) 2019. tangzx(love.tangzx@qq.com)
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/




#if  defined(EMMY_LUA_51) || defined(EMMY_LUA_JIT)
#include "emmy_debugger/api/lua_api.h"
int lua_absindex(lua_State *L, int idx) {
	if (idx > 0) {
		return idx;
	}
	return lua_gettop(L) + idx + 1;
}
#ifndef EMMY_LUA_JIT_SUPPORT_LUA_SETFUNCS
void luaL_setfuncs(lua_State* L, const luaL_Reg* l, int nup) {
	for (; l->name != nullptr; l++) {
		for (int i = 0; i < nup; i++)
			lua_pushvalue(L, -nup);
		lua_pushcclosure(L, l->func, nup);
		lua_setfield(L, -(nup + 2), l->name);
	}
}
#endif

void lua_pushglobaltable(lua_State* L)
{
	lua_pushvalue(L, LUA_GLOBALSINDEX);
}

#endif
