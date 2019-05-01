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

#define EMMY_CORE_VERSION "1.0.0"

#ifdef _MSC_VER
#define EMMY_CORE_EXPORT    __declspec(dllexport)
#else
#define EMMY_CORE_EXPORT    extern
#endif

#ifndef EMMY_USE_LUA_SOURCE
    #include "lua_copy/lua_api.h"
#else//EMMY_USE_LUA_SOURCE
    extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
    }

    #ifdef EMMY_LUA_51
        #define LUA_OK 0
    #endif
#endif//EMMY_USE_LUA_SOURCE

// lua version
enum class LuaVersion {
    UNKNOWN, LUA_51 = 51, LUA_52 = 52, LUA_53 = 53
};

extern LuaVersion luaVersion;
