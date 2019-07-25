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
#include "../emmy_core.h"
#include <cstdio>

#ifdef WIN32
#include <Windows.h>
#include <TlHelp32.h>

HMODULE FindLuaModule() {
	HMODULE hModule = nullptr;
	MODULEENTRY32 module;
	module.dwSize = sizeof(MODULEENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	BOOL moreModules = Module32First(hSnapshot, &module);

	while (moreModules) {
		if (GetProcAddress(module.hModule, "lua_gettop")) {
			hModule = module.hModule;
			break;
		}
		moreModules = Module32Next(hSnapshot, &module);
	}
	return hModule;
}

HMODULE hModule = FindLuaModule();

FARPROC LoadAPI(const char* name) {
	return GetProcAddress(hModule, name);
}
#else
#include <dlfcn.h>
void* LoadAPI(const char* name) {
    const auto handler = dlsym(RTLD_DEFAULT, name);
    return handler;
}
#endif

IMPL_LUA_API(lua_gettop);
IMPL_LUA_API(lua_settop);
IMPL_LUA_API(lua_type);
IMPL_LUA_API(lua_typename);
IMPL_LUA_API(lua_tolstring);
IMPL_LUA_API(lua_toboolean);
IMPL_LUA_API(lua_pushnil);
IMPL_LUA_API(lua_pushnumber);
IMPL_LUA_API(lua_pushlstring);
IMPL_LUA_API(lua_pushstring);
IMPL_LUA_API(lua_pushcclosure);
IMPL_LUA_API(lua_pushboolean);
IMPL_LUA_API(lua_pushvalue);
IMPL_LUA_API(lua_getfield);
IMPL_LUA_API(lua_next);
IMPL_LUA_API(lua_createtable);
IMPL_LUA_API(lua_setfield);
IMPL_LUA_API(lua_setmetatable);
IMPL_LUA_API(lua_getstack);
IMPL_LUA_API(lua_getinfo);
IMPL_LUA_API(lua_getlocal);
IMPL_LUA_API(lua_getupvalue);
IMPL_LUA_API(lua_setupvalue);
IMPL_LUA_API(lua_sethook);
IMPL_LUA_API(luaL_loadstring);
IMPL_LUA_API(luaL_checklstring);
IMPL_LUA_API(luaL_checknumber);
IMPL_LUA_API(lua_topointer);
IMPL_LUA_API(lua_getmetatable);
IMPL_LUA_API(lua_rawget);
IMPL_LUA_API(lua_rawset);
IMPL_LUA_API(lua_pushlightuserdata);
IMPL_LUA_API(lua_touserdata);
IMPL_LUA_API(lua_newuserdata);
IMPL_LUA_API(lua_rawseti);
IMPL_LUA_API(lua_rawgeti);
//51
IMPL_LUA_API_E(lua_setfenv);
IMPL_LUA_API_E(lua_tointeger);
IMPL_LUA_API_E(lua_tonumber);
IMPL_LUA_API_E(lua_call);
IMPL_LUA_API_E(lua_pcall);
IMPL_LUA_API_E(lua_remove);
//53
IMPL_LUA_API_E(lua_tointegerx);
IMPL_LUA_API_E(lua_tonumberx);
IMPL_LUA_API_E(lua_getglobal);
IMPL_LUA_API_E(lua_callk);
IMPL_LUA_API_E(lua_pcallk);
IMPL_LUA_API_E(luaL_setfuncs);
IMPL_LUA_API_E(lua_absindex);
IMPL_LUA_API_E(lua_rotate);


int lua_setfenv(lua_State* L, int idx) {
	if (luaVersion == LuaVersion::LUA_51) {
		return e_lua_setfenv(L, idx);
	}
	return lua_setupvalue(L, idx, 1) != nullptr;
}

lua_Integer lua_tointeger(lua_State* L, int idx) {
	if (luaVersion > LuaVersion::LUA_51) {
		return e_lua_tointegerx(L, idx, nullptr);
	}
	return e_lua_tointeger(L, idx);
}

lua_Number lua_tonumber(lua_State* L, int idx) {
	if (luaVersion == LuaVersion::LUA_51) {
		return e_lua_tonumber(L, idx);
	}
	return e_lua_tonumberx(L, idx, nullptr);
}

int lua_getglobal(lua_State* L, const char* name) {
	if (luaVersion == LuaVersion::LUA_51) {
		return lua_getfield(L, -10002, name);
	}
	return e_lua_getglobal(L, name);
}

void lua_call(lua_State* L, int nargs, int nresults) {
	if (luaVersion == LuaVersion::LUA_51)
		e_lua_call(L, nargs, nresults);
	else
		e_lua_callk(L, nargs, nresults, 0, nullptr);
}

int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc) {
	if (luaVersion == LuaVersion::LUA_51)
		return e_lua_pcall(L, nargs, nresults, errfunc);
	return e_lua_pcallk(L, nargs, nresults, errfunc, 0, nullptr);
}

void luaL_setfuncs(lua_State* L, const luaL_Reg* l, int nup) {
	if (luaVersion == LuaVersion::LUA_51) {
		for (; l->name != nullptr; l++) {
			int i;
			for (i = 0; i < nup; i++)
				lua_pushvalue(L, -nup);
			lua_pushcclosure(L, l->func, nup);
			lua_setfield(L, -(nup + 2), l->name);
		}
	}
	else e_luaL_setfuncs(L, l, nup);
}

int lua_upvalueindex(int i) {
	if (luaVersion == LuaVersion::LUA_53)
		return -1001000 - i;
	if (luaVersion == LuaVersion::LUA_52)
		return -1001000 - i;
	return -10002 - i;
}

int lua_absindex(lua_State *L, int idx) {
	if (luaVersion == LuaVersion::LUA_51) {
		if (idx > 0) {
			return idx;
		}
		return lua_gettop(L) + idx + 1;
	}
	return e_lua_absindex(L, idx);
}

void lua_remove(lua_State *L, int idx) {
	if (luaVersion == LuaVersion::LUA_51 || luaVersion == LuaVersion::LUA_52) {
		e_lua_remove(L, idx);
	}
	else {
		e_lua_rotate(L, (idx), -1);
		lua_pop(L, 1);
	}
}

int LUA_REGISTRYINDEX = 0;

extern "C" bool SetupLuaAPI() {
	REQUIRE_LUA_API(lua_gettop);
	REQUIRE_LUA_API(lua_settop);
	REQUIRE_LUA_API(lua_type);
	REQUIRE_LUA_API(lua_typename);
	REQUIRE_LUA_API(lua_tolstring);
	REQUIRE_LUA_API(lua_toboolean);
	REQUIRE_LUA_API(lua_pushnil);
	REQUIRE_LUA_API(lua_pushnumber);
	REQUIRE_LUA_API(lua_pushlstring);
	REQUIRE_LUA_API(lua_pushstring);
	REQUIRE_LUA_API(lua_pushcclosure);
	REQUIRE_LUA_API(lua_pushboolean);
	REQUIRE_LUA_API(lua_pushvalue);
	REQUIRE_LUA_API(lua_getfield);
	REQUIRE_LUA_API(lua_next);
	REQUIRE_LUA_API(lua_createtable);
	REQUIRE_LUA_API(lua_setfield);
	REQUIRE_LUA_API(lua_setmetatable);
	REQUIRE_LUA_API(lua_getstack);
	REQUIRE_LUA_API(lua_getinfo);
	REQUIRE_LUA_API(lua_getlocal);
	REQUIRE_LUA_API(lua_getupvalue);
	REQUIRE_LUA_API(lua_setupvalue);
	REQUIRE_LUA_API(lua_sethook);
	REQUIRE_LUA_API(luaL_loadstring);
	REQUIRE_LUA_API(luaL_checklstring);
	REQUIRE_LUA_API(luaL_checknumber);
	REQUIRE_LUA_API(lua_topointer);
	REQUIRE_LUA_API(lua_getmetatable);
	REQUIRE_LUA_API(lua_rawget);
	REQUIRE_LUA_API(lua_rawset);
	REQUIRE_LUA_API(lua_pushlightuserdata);
	REQUIRE_LUA_API(lua_touserdata);
	REQUIRE_LUA_API(lua_newuserdata);
	REQUIRE_LUA_API(lua_rawseti);
	REQUIRE_LUA_API(lua_rawgeti);
	//51
	REQUIRE_LUA_API_E(lua_setfenv);
	REQUIRE_LUA_API_E(lua_tointeger);
	REQUIRE_LUA_API_E(lua_tonumber);
	REQUIRE_LUA_API_E(lua_call);
	REQUIRE_LUA_API_E(lua_pcall);
	//51 & 52
	REQUIRE_LUA_API_E(lua_remove);
	//52 & 53
	REQUIRE_LUA_API_E(lua_tointegerx);
	REQUIRE_LUA_API_E(lua_tonumberx);
	REQUIRE_LUA_API_E(lua_getglobal);
	REQUIRE_LUA_API_E(lua_callk);
	REQUIRE_LUA_API_E(lua_pcallk);
	REQUIRE_LUA_API_E(luaL_setfuncs);
	REQUIRE_LUA_API_E(lua_absindex);
	//53
	REQUIRE_LUA_API_E(lua_rotate);

	if (e_lua_rotate) {
		luaVersion = LuaVersion::LUA_53;
		LUA_REGISTRYINDEX = -1001000;
	}
	else if (e_lua_callk) {
		luaVersion = LuaVersion::LUA_52;
		//todo
		LUA_REGISTRYINDEX = -1001000;
	}
	else {
		luaVersion = LuaVersion::LUA_51;
		LUA_REGISTRYINDEX = -10000;
	}
	printf("[EMMY]lua version: %d\n", luaVersion);
	return true;
}
