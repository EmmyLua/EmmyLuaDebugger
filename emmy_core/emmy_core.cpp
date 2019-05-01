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

// emmy.tcpListen(host: string, port: int): bool
int tcpListen(struct lua_State* L) {
    luaL_checkstring(L, 1);
    const auto host = lua_tostring(L, 1);
    luaL_checknumber(L, 2);
    const auto port = lua_tointeger(L, 2);
	const auto suc = EmmyFacade::Get()->TcpListen(L, host, port) == 0;
    lua_pushboolean(L, suc);
	return 1;
}

// emmy.tcpConnect(host: string, port: int): bool
int tcpConnect(lua_State* L) {
    luaL_checkstring(L, 1);
    const auto host = lua_tostring(L, 1);
    luaL_checknumber(L, 2);
    const auto port = lua_tointeger(L, 2);
    const auto suc = EmmyFacade::Get()->TcpConnect(L, host, port) == 0;
    lua_pushboolean(L, suc);
    return 1;
}

// emmy.pipeListen(pipeName: string): bool
int pipeListen(lua_State* L) {
    luaL_checkstring(L, 1);
    const auto pipeName = lua_tostring(L, 1);
    const auto suc = EmmyFacade::Get()->PipeListen(L, pipeName) == 0;
    lua_pushboolean(L, suc);
    return 1;
}

// emmy.pipeConnect(pipeName: string): bool
int pipeConnect(lua_State* L) {
    luaL_checkstring(L, 1);
    const auto pipeName = lua_tostring(L, 1);
    const auto suc = EmmyFacade::Get()->PipeConnect(L, pipeName) == 0;
    lua_pushboolean(L, suc);
    return 1;
}

// emmy.breakHere(): bool
int breakHere(lua_State* L) {
	const bool suc = EmmyFacade::Get()->BreakHere(L);
	lua_pushboolean(L, suc);
	return 1;
}

// emmy.waitIDE(): void
int waitIDE(lua_State* L) {
    EmmyFacade::Get()->WaitIDE();
    return 0;
}

int gc(lua_State* L) {
    //EmmyFacade::Get()->Destroy();
    return 0;
}

static const luaL_Reg lib[] = {
    {"tcpListen", tcpListen},
    {"tcpConnect", tcpConnect},
    {"pipeListen", pipeListen},
    {"pipeConnect", pipeConnect},
    {"waitIDE", waitIDE},
	{"breakHere", breakHere},
	{nullptr, nullptr}
};

#if EMMY_LUA_51
LuaVersion luaVersion = LuaVersion::LUA_51;
#elif EMMY_LUA_52
LuaVersion luaVersion = LuaVersion::LUA_52;
#elif EMMY_LUA_53
LuaVersion luaVersion = LuaVersion::LUA_53;
#else
LuaVersion luaVersion = LuaVersion::UNKNOWN;
#endif

extern "C" {
EMMY_CORE_EXPORT int luaopen_emmy_core(struct lua_State* L) {
#ifndef EMMY_USE_LUA_SOURCE
    extern bool SetupLuaAPI();
	if (!SetupLuaAPI()) {
		return 0;
	}
#endif
    luaL_newlib(L, lib);
    
    // setmetatable(lib, { __gc = gc })
    lua_newtable(L);
    lua_pushcfunction(L, gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -1);

	// reg["__emmy__"] = lib
	lua_pushvalue(L, -1);
	lua_setfield(L, LUA_REGISTRYINDEX, "$__emmy__$");

	return 1;
}
}
