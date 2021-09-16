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
#pragma once

#ifdef EMMY_USE_LUA_SOURCE
///////////////////////////EMMY_USE_LUA_SOURCE
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#if defined(EMMY_LUA_51) || defined(EMMY_LUA_52)
typedef ptrdiff_t lua_KContext;
/*
** Type for continuation functions
*/
typedef int(*lua_KFunction) (lua_State *L, int status, lua_KContext ctx);
#endif

#if defined(EMMY_LUA_51) || defined(EMMY_LUA_JIT)
#define LUA_OK 0

int lua_absindex(lua_State *L, int idx);

#ifndef EMMY_LUA_JIT_SUPPORT_LUA_SETFUNCS
void luaL_setfuncs(lua_State* L, const luaL_Reg* l, int nup);
#endif

#ifndef luaL_newlibtable
#define luaL_newlibtable(L,l)	\
  lua_createtable(L, 0, sizeof(l)/sizeof((l)[0]) - 1)
#endif

#ifndef luaL_newlib
#define luaL_newlib(L,l)  \
  (luaL_newlibtable(L,l), luaL_setfuncs(L,l,0))
#endif

void lua_pushglobaltable(lua_State* L);

#endif

inline int getDebugEvent(lua_Debug* ar) {
	return  ar->event;
}

inline int getDebugCurrentLine(lua_Debug* ar) {
	return  ar->currentline;
}

inline const char* getDebugName(lua_Debug* ar) {
	return ar->name;
}

inline const char* getDebugSource(lua_Debug* ar) {
	return ar->source;
}
#include "lua_state.h"
///////////////////////////EMMY_USE_LUA_SOURCE
#else
#include "lua_api_loader.h"
#endif