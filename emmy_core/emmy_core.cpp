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
#include "emmy_core.h"
#include "emmy_facade.h"

int luaopen_emmy_helper(lua_State* L);

// emmy.tcpListen(host: string, port: int): bool
int tcpListen(struct lua_State* L) {
	luaL_checkstring(L, 1);
	std::string err;
	const auto host = lua_tostring(L, 1);
	luaL_checknumber(L, 2);
	const auto port = lua_tointeger(L, 2);
	const auto suc = EmmyFacade::Get()->TcpListen(L, host, port, err);
	lua_pushboolean(L, suc);
	if (suc) return 1;
	lua_pushstring(L, err.c_str());
	return 2;
}

// emmy.tcpConnect(host: string, port: int): bool
int tcpConnect(lua_State* L) {
	luaL_checkstring(L, 1);
	std::string err;
	const auto host = lua_tostring(L, 1);
	luaL_checknumber(L, 2);
	const auto port = lua_tointeger(L, 2);
	const auto suc = EmmyFacade::Get()->TcpConnect(L, host, port, err);
	lua_pushboolean(L, suc);
	if (suc) return 1;
	lua_pushstring(L, err.c_str());
	return 2;
}

// emmy.pipeListen(pipeName: string): bool
int pipeListen(lua_State* L) {
	luaL_checkstring(L, 1);
	std::string err;
	const auto pipeName = lua_tostring(L, 1);
	const auto suc = EmmyFacade::Get()->PipeListen(L, pipeName, err);
	lua_pushboolean(L, suc);
	if (suc) return 1;
	lua_pushstring(L, err.c_str());
	return 2;
}

// emmy.pipeConnect(pipeName: string): bool
int pipeConnect(lua_State* L) {
	luaL_checkstring(L, 1);
	std::string err;
	const auto pipeName = lua_tostring(L, 1);
	const auto suc = EmmyFacade::Get()->PipeConnect(L, pipeName, err);
	lua_pushboolean(L, suc);
	if (suc) return 1;
	lua_pushstring(L, err.c_str());
	return 2;
}

// emmy.breakHere(): bool
int breakHere(lua_State* L) {
	const bool suc = EmmyFacade::Get()->BreakHere(L);
	lua_pushboolean(L, suc);
	return 1;
}

// emmy.waitIDE(timeout: number): void
int waitIDE(lua_State* L) {
	int top = lua_gettop(L);
	int timeout = 0;
	if (top >= 1) timeout = luaL_checknumber(L, 1);
	EmmyFacade::Get()->WaitIDE(false, timeout);
	return 0;
}

// emmy.stop()
int stop(lua_State* L) {
	EmmyFacade::Get()->Destroy();
	return 0;
}

int gc(lua_State* L) {
	EmmyFacade::Get()->OnLuaStateGC(L);
	return 0;
}

void handleStateClose(lua_State* L) {
	lua_newtable(L);
	lua_pushcfunction(L, gc);
	lua_setfield(L, -2, "__gc");
	
	lua_newuserdata(L, 1);
	lua_pushvalue(L, -2);
	lua_setmetatable(L, -2);
	
	lua_setfield(L, LUA_REGISTRYINDEX, "__EMMY__GC__");
	
	lua_pop(L, 1);
}

static const luaL_Reg lib[] = {
	{"tcpListen", tcpListen},
	{"tcpConnect", tcpConnect},
	{"pipeListen", pipeListen},
	{"pipeConnect", pipeConnect},
	{"waitIDE", waitIDE},
	{"breakHere", breakHere},
	{"stop", stop},
	{nullptr, nullptr}
};

#if EMMY_LUA_51
LuaVersion luaVersion = LuaVersion::LUA_51;
#elif EMMY_LUA_52
LuaVersion luaVersion = LuaVersion::LUA_52;
#elif EMMY_LUA_53
LuaVersion luaVersion = LuaVersion::LUA_53;
#elif EMMY_LUA_54
LuaVersion luaVersion = LuaVersion::LUA_54;
#else
LuaVersion luaVersion = LuaVersion::UNKNOWN;
#endif

extern "C" {
	bool install_emmy_core(struct lua_State* L) {
#ifndef EMMY_USE_LUA_SOURCE
		if (!EmmyFacade::Get()->SetupLuaAPI()) {
			return false;
		}
#endif
		// register helper lib
		luaopen_emmy_helper(L);
		handleStateClose(L);
		return true;
	}

	EMMY_CORE_EXPORT int luaopen_emmy_core(struct lua_State* L) {
		if (!install_emmy_core(L))
			return false;
		luaL_newlibtable(L, lib);
		luaL_setfuncs(L, lib, 0);

		// _G.emmy_core
		lua_pushvalue(L, -1);
		lua_setglobal(L, "emmy_core");
		
		return 1;
	}
}
