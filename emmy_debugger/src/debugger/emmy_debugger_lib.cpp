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
#include "emmy_debugger/debugger/emmy_debugger_lib.h"
#include <cstring>
#include "emmy_debugger/debugger/emmy_debugger.h"
#include "emmy_debugger/emmy_facade.h"

// emmy.tcpListen(host: string, port: int): bool
int tcpListen(struct lua_State* L)
{
	luaL_checkstring(L, 1);
	std::string err;
	const auto host = lua_tostring(L, 1);
	luaL_checknumber(L, 2);
	const auto port = lua_tointeger(L, 2);
	const auto suc = EmmyFacade::Get().TcpListen(L, host, static_cast<int>(port), err);
	lua_pushboolean(L, suc);
	if (suc) return 1;
	lua_pushstring(L, err.c_str());
	return 2;
}

// emmy.tcpConnect(host: string, port: int): bool
int tcpConnect(lua_State* L)
{
	luaL_checkstring(L, 1);
	std::string err;
	const auto host = lua_tostring(L, 1);
	luaL_checknumber(L, 2);
	const auto port = lua_tointeger(L, 2);
	const auto suc = EmmyFacade::Get().TcpConnect(L, host, static_cast<int>(port), err);
	lua_pushboolean(L, suc);
	if (suc) return 1;
	lua_pushstring(L, err.c_str());
	return 2;
}

// emmy.pipeListen(pipeName: string): bool
int pipeListen(lua_State* L)
{
	luaL_checkstring(L, 1);
	std::string err;
	const auto pipeName = lua_tostring(L, 1);
	const auto suc = EmmyFacade::Get().PipeListen(L, pipeName, err);
	lua_pushboolean(L, suc);
	if (suc) return 1;
	lua_pushstring(L, err.c_str());
	return 2;
}

// emmy.pipeConnect(pipeName: string): bool
int pipeConnect(lua_State* L)
{
	luaL_checkstring(L, 1);
	std::string err;
	const auto pipeName = lua_tostring(L, 1);
	const auto suc = EmmyFacade::Get().PipeConnect(L, pipeName, err);
	lua_pushboolean(L, suc);
	if (suc) return 1;
	lua_pushstring(L, err.c_str());
	return 2;
}

// emmy.breakHere(): bool
int breakHere(lua_State* L)
{
	const bool suc = EmmyFacade::Get().BreakHere(L);
	lua_pushboolean(L, suc);
	return 1;
}

// emmy.waitIDE(timeout: number): void
int waitIDE(lua_State* L)
{
	int top = lua_gettop(L);
	int timeout = 0;
	if (top >= 1)
	{
		timeout = static_cast<int>(luaL_checknumber(L, 1));
	}
	EmmyFacade::Get().WaitIDE(false, timeout);
	return 0;
}

int tcpSharedListen(lua_State* L)
{
	luaL_checkstring(L, 1);
	std::string err;
	const auto host = lua_tostring(L, 1);
	luaL_checknumber(L, 2);
	const auto port = lua_tointeger(L, 2);
	const auto suc = EmmyFacade::Get().TcpSharedListen(L, host, static_cast<int>(port), err);
	lua_pushboolean(L, suc);
	if (suc) return 1;
	lua_pushstring(L, err.c_str());
	return 2;
}


// emmy.stop()
int stop(lua_State* L)
{
	EmmyFacade::Get().Destroy();
	return 0;
}

// emmy.registerTypeName(typeName: string): bool
int registerTypeName(lua_State* L)
{
	luaL_checkstring(L, 1);
	std::string err;
	const auto typeName = lua_tostring(L, 1);
	const auto suc = EmmyFacade::Get().RegisterTypeName(L, typeName, err);
	lua_pushboolean(L, suc);
	if (suc) return 1;
	lua_pushstring(L, err.c_str());
	return 2;
}

int gc(lua_State* L)
{
	EmmyFacade::Get().OnLuaStateGC(L);
	return 0;
}

void handleStateClose(lua_State* L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, "__EMMY__GC__");
	int isNil = lua_isnil(L, -1);
	lua_pop(L, 1);

	if (!isNil)
	{
		return;
	}

	lua_newtable(L);
	lua_pushcfunction(L, gc);
	lua_setfield(L, -2, "__gc");

	lua_newuserdata(L, 1);
	lua_pushvalue(L, -2);
	lua_setmetatable(L, -2);

	lua_setfield(L, LUA_REGISTRYINDEX, "__EMMY__GC__");

	lua_pop(L, 1);
}


bool install_emmy_debugger(struct lua_State* L)
{
#ifndef EMMY_USE_LUA_SOURCE
	if (!EmmyFacade::Get().SetupLuaAPI())
	{
		return false;
	}
#endif
	// register helper lib
	EmmyFacade::Get().GetDebugManager().extension.Initialize(L);
	handleStateClose(L);
	return true;
}

std::string prepareEvalExpr(const std::string& eval)
{
	if (eval.empty())
	{
		return eval;
	}

	int lastIndex = static_cast<int>(eval.size() - 1);

	for (int i = lastIndex; i >= 0; i--)
	{
		auto ch = eval[i];
		if (ch > 0)
		{
			if (!isalnum(ch) && ch != '_')
			{
				if (ch == ':')
				{
					auto newString = eval;
					newString[i] = '.';
					return eval;
				}
			}
		}
	}
	return eval;
}
