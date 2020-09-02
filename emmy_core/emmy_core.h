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

#include "api/lua_api.h"

// lua version
enum class LuaVersion {
	UNKNOWN,
	LUA_51 = 51,
	LUA_52 = 52,
	LUA_53 = 53,
	LUA_54 = 54
};

extern LuaVersion luaVersion;