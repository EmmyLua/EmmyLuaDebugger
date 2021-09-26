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
#include "emmy_core/emmy_core.h"
#include "emmy_debugger/emmy_helper.h"
#include "emmy_debugger/emmy_facade.h"

static const luaL_Reg lib[] = {
	{"tcpListen", tcpListen},
	{"tcpConnect", tcpConnect},
	{"pipeListen", pipeListen},
	{"pipeConnect", pipeConnect},
	{"waitIDE", waitIDE},
	{"breakHere", breakHere},
	{"stop", stop},
	{"tcpSharedListen", tcpSharedListen},
	{nullptr, nullptr}
};

extern "C" {
	EMMY_CORE_EXPORT int luaopen_emmy_core(struct lua_State* L) {
		EmmyFacade::Get().SetWorkMode(WorkMode::EmmyCore);
		if (!install_emmy_core(L))
			return false;
		luaL_newlibtable(L, lib);
		luaL_setfuncs(L, lib, 0);

		// _G.emmy_core
		lua_pushglobaltable(L);
		lua_pushstring(L, "emmy_core");
		lua_pushvalue(L, -3);
		lua_rawset(L, -3);
		lua_pop(L, 1);
		
		return 1;
	}
}
