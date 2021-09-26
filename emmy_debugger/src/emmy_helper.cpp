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
#include <cstring>
#include "emmy_debugger/types.h"
#include "emmy_debugger/emmy_debugger.h"
#include "emmy_debugger/emmy_facade.h"


#define HELPER_NAME "emmyHelper"

bool initialized = false;

int metaQuery(lua_State* L)
{
	const int argN = lua_gettop(L);
	auto* var = (Variable*)lua_touserdata(L, 1);
	const int index = 2;
	const auto depth = lua_tonumber(L, 3);
	bool queryHelper = false;
	if (argN >= 4)
	{
		queryHelper = lua_toboolean(L, 4);
	}
	auto debugger = EmmyFacade::Get().GetDebugger(L);
	auto variable = EmmyFacade::Get().GetVariableRef(var);

	if (debugger && variable)
	{
		debugger->GetVariable(variable, index, static_cast<int>(depth), queryHelper);
	}
	return 0;
}

int metaAddChild(lua_State* L)
{
	auto* var = (Variable*)lua_touserdata(L, 1);
	auto* child = (Variable*)lua_touserdata(L, 2);
	auto childRef = EmmyFacade::Get().GetVariableRef(child);
	if (childRef)
	{
		var->children.push_back(childRef);
	}

	return 0;
}

int metaIndex(lua_State* L)
{
	const Variable* var = (Variable*)lua_touserdata(L, 1);
	const std::string k = lua_tostring(L, 2);
	if (k == "name")
	{
		lua_pushstring(L, var->name.c_str());
	}
	else if (k == "value")
	{
		lua_pushstring(L, var->value.c_str());
	}
	else if (k == "valueType")
	{
		lua_pushnumber(L, var->valueType);
	}
	else if (k == "valueTypeName")
	{
		lua_pushstring(L, var->valueTypeName.c_str());
	}
	else if (k == "addChild")
	{
		lua_pushcfunction(L, metaAddChild);
	}
	else if (k == "query")
	{
		lua_pushcfunction(L, metaQuery);
	}
	else
	{
		lua_pushnil(L);
	}
	return 1;
}

int metaNewIndex(lua_State* L)
{
	auto var = (Variable*)lua_touserdata(L, 1);
	const std::string k = lua_tostring(L, 2);
	if (k == "name")
	{
		const char* value = lua_tostring(L, 3);
		var->name = value ? value : "";
	}
	else if (k == "value")
	{
		const char* value = lua_tostring(L, 3);
		var->value = value ? value : "";
	}
	else if (k == "valueType")
	{
		var->valueType = static_cast<int>(lua_tonumber(L, 3));
	}
	else if (k == "valueTypeName")
	{
		const char* value = lua_tostring(L, 3);
		var->valueTypeName = value ? value : "";
	}
	return 0;
}

int metaGC(lua_State* L)
{
	const auto var = (Variable*)lua_touserdata(L, 1);
	EmmyFacade::Get().RemoveVariableRef(var);

	return 0;
}

void setupMeta(lua_State* L)
{
	lua_newtable(L);

	lua_pushcfunction(L, metaIndex);
	lua_setfield(L, -2, "__index");

	lua_pushcfunction(L, metaNewIndex);
	lua_setfield(L, -2, "__newindex");

	lua_pushcfunction(L, metaGC);
	lua_setfield(L, -2, "__gc");

	lua_setmetatable(L, -2);
}

int createNode(lua_State* L)
{
	auto variable = std::make_shared<Variable>();
	EmmyFacade::Get().AddVariableRef(variable);
	lua_pushlightuserdata(L, variable.get());
	setupMeta(L);
	return 1;
}


bool query_variable(lua_State* L, std::shared_ptr<Variable> variable, const char* typeName, int object, int depth)
{
	if (!initialized)
	{
		return false;
	}
	bool result = false;
	object = lua_absindex(L, object);
	const int t = lua_gettop(L);
	lua_getglobal(L, HELPER_NAME);

	if (lua_istable(L, -1))
	{
		lua_getfield(L, -1, "queryVariable");
		if (lua_isfunction(L, -1))
		{
			EmmyFacade::Get().AddVariableRef(variable);

			lua_pushlightuserdata(L, variable.get());
			setupMeta(L);
			lua_pushvalue(L, object);
			lua_pushstring(L, typeName);
			lua_pushnumber(L, depth);
			const auto r = lua_pcall(L, 4, 1, 0);
			if (r == LUA_OK)
			{
				result = lua_toboolean(L, -1);
			}
			else
			{
				const auto err = lua_tostring(L, -1);
				printf("query error: %s\n", err);
			}
		}
	}

	lua_settop(L, t);
	return result;
}

// emmy_init(emmy)
int emmyHelperInit(lua_State* L)
{
	lua_getglobal(L, HELPER_NAME);
	if (lua_istable(L, -1))
	{
		// create node
		lua_pushcfunction(L, createNode);
		lua_setfield(L, -2, "createNode");

		initialized = true;
	}
	return 0;
}

// 1: table
int luaopen_emmy_helper(lua_State* L)
{
	initialized = false;
	lua_pushglobaltable(L);

	lua_pushstring(L, "emmyHelperInit");
	lua_pushcfunction(L, emmyHelperInit);
	lua_rawset(L, -3);
	lua_pop(L, 1);

	return 0;
}

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

	if(!isNil)
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


bool install_emmy_core(struct lua_State* L)
{
#ifndef EMMY_USE_LUA_SOURCE
	if (!EmmyFacade::Get().SetupLuaAPI())
	{
		return false;
	}
#endif
	// register helper lib
	luaopen_emmy_helper(L);
	handleStateClose(L);
	return true;
}


void ParsePathParts(const std::string& file, std::vector<std::string>& paths)
{
	size_t idx = 0;
	for (size_t i = 0; i < file.length(); i++)
	{
		const char c = file.at(i);
		if (c == '/' || c == '\\')
		{
			const auto part = file.substr(idx, i - idx);
			idx = i + 1;
			// ../a/b/c
			if (part == "..")
			{
				if (!paths.empty()) paths.pop_back();
				continue;
			}
			// ./a/b/c
			if ((part == "." || part.empty()) && paths.empty())
			{
				continue;
			}
			paths.emplace_back(part);
		}
	}
	// file name
	paths.emplace_back(file.substr(idx));
}

// glibc 函数
int __strncasecmp(const char* s1, const char* s2, int n)
{
	if (n && s1 != s2)
	{
		do
		{
			int d = ::tolower(*s1) - ::tolower(*s2);
			if (d || *s1 == '\0' || *s2 == '\0') return d;
			s1++;
			s2++;
		}
		while (--n);
	}
	return 0;
}

bool CompareIgnoreCase(const std::string& lh, const std::string& rh)
{
	std::size_t llen = lh.size();
	std::size_t rlen = rh.size();
	int ret = __strncasecmp(lh.data(), rh.data(), static_cast<int>((std::min)(llen, rlen)));
	return ret;
}

bool CaseInsensitiveLess::operator()(const std::string& lhs, const std::string& rhs) const
{
	std::size_t llen = lhs.size();
	std::size_t rlen = rhs.size();

	int ret = CompareIgnoreCase(lhs, rhs);

	if (ret < 0)
	{
		return true;
	}
	if (ret == 0 && llen < rlen)
	{
		return true;
	}
	return false;
}

bool EndWith(const std::string& source, const std::string& end)
{
	auto endSize = end.size();
	auto sourceSize = source.size();

	if (endSize > sourceSize)
	{
		return false;
	}

	return strncmp(end.data(), source.data() + (sourceSize - endSize), endSize) == 0;
}

std::string BaseName(const std::string& filePath)
{
	std::size_t sepIndex = filePath.find_last_of('/');
	if (sepIndex == std::string::npos)
	{
		sepIndex = filePath.find_last_of('\\');
		if (sepIndex != std::string::npos)
		{
			return filePath.substr(sepIndex + 1);
		}
		return filePath;
	}
	else
	{
		return filePath.substr(sepIndex + 1);
	}
}
