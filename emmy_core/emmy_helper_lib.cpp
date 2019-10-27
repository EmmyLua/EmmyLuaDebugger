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
#include "types.h"
#include "emmy_debugger.h"

#define HELPER_NAME "emmyHelper"

bool initialized = false;

int metaQuery(lua_State* L) {
	const int argN = lua_gettop(L);
	auto* var = (Variable*)lua_touserdata(L, 1);
	const int index = 2;
	const int depth = lua_tonumber(L, 3);
	bool queryHelper = false;
	if (argN >= 4) {
		queryHelper = lua_toboolean(L, 4);
	}
	Debugger::Get()->GetVariable(var, L, index, depth, queryHelper);
	return 0;
}

int metaAddChild(lua_State* L) {
	auto* var = (Variable*)lua_touserdata(L, 1);
	auto* child = (Variable*)lua_touserdata(L, 2);
	var->children.push_back(child);
	return 0;
}

int metaIndex(lua_State* L) {
	const Variable* var = (Variable*)lua_touserdata(L, 1);
	const std::string k = lua_tostring(L, 2);
	if (k == "name") {
		lua_pushstring(L, var->name.c_str());
	}
	else if (k == "value") {
		lua_pushstring(L, var->value.c_str());
	}
	else if (k == "valueType") {
		lua_pushnumber(L, var->valueType);
	}
	else if (k == "valueTypeName") {
		lua_pushstring(L, var->valueTypeName.c_str());
	}
	else if (k == "addChild") {
		lua_pushcfunction(L, metaAddChild);
	}
	else if (k == "query") {
		lua_pushcfunction(L, metaQuery);
	}
	else {
		lua_pushnil(L);
	}
	return 1;
}

int metaNewIndex(lua_State* L) {
	auto var = (Variable*)lua_touserdata(L, 1);
	const std::string k = lua_tostring(L, 2);
	if (k == "name") {
		const char* value = lua_tostring(L, 3);
		var->name = value ? value : "";
	}
	else if (k == "value") {
		const char* value = lua_tostring(L, 3);
		var->value = value ? value : "";
	}
	else if (k == "valueType") {
		var->valueType = lua_tonumber(L, 3);
	}
	else if (k == "valueTypeName") {
		const char* value = lua_tostring(L, 3);
		var->valueTypeName = value ? value : "";
	}
	return 0;
}

int metaGC(lua_State* L) {
	const auto var = (Variable*)lua_touserdata(L, 1);
	delete var;
	return 0;
}

void setupMeta(lua_State* L) {
	lua_newtable(L);

	lua_pushcfunction(L, metaIndex);
	lua_setfield(L, -2, "__index");

	lua_pushcfunction(L, metaNewIndex);
	lua_setfield(L, -2, "__newindex");

	lua_pushcfunction(L, metaGC);
	lua_setfield(L, -2, "__gc");

	lua_setmetatable(L, -2);
}

int createNode(lua_State* L) {
	const Variable* var = new Variable();
	lua_pushlightuserdata(L, (void*)var);
	setupMeta(L);
	return 1;
}

bool query_variable(Variable* variable, lua_State* L, const char* typeName, int object, int depth) {
	if (!initialized) {
		return false;
	}
	bool result = false;
	object = lua_absindex(L, object);
	const int t = lua_gettop(L);
	lua_getglobal(L, HELPER_NAME);
	lua_getfield(L, -1, "queryVariable");
	if (lua_isfunction(L, -1)) {
		lua_pushlightuserdata(L, variable);
		setupMeta(L);
		lua_pushvalue(L, object);
		lua_pushstring(L, typeName);
		lua_pushnumber(L, depth);
		const auto r = lua_pcall(L, 4, 1, 0);
		if (r == LUA_OK) {
			result = lua_toboolean(L, -1);
		}
		else {
			const auto err = lua_tostring(L, -1);
			printf("query error: %s\n", err);
		}
	}
	lua_settop(L, t);
	return result;
}

// emmy_init(emmy)
int emmyHelperInit(lua_State* L) {
	lua_getglobal(L, HELPER_NAME);
	if (lua_istable(L, -1)) {
		// create node
		lua_pushcfunction(L, createNode);
		lua_setfield(L, -2, "createNode");

		initialized = true;
	}
	return 0;
}

// 1: table
int luaopen_emmy_helper(lua_State* L) {
	initialized = false;

	lua_pushcfunction(L, emmyHelperInit);
    lua_setglobal(L, "emmyHelperInit");

	return 0;
}