#pragma once

#include <string>
#include "emmy_debugger/api/lua_api.h"
#include "emmy_debugger/proto/proto.h"

class ExtensionPoint {
public:
	static std::string ExtensionTable;

	ExtensionPoint();

	void Initialize(lua_State *L);
	// 
	bool QueryVariable(lua_State *L, Idx<Variable> variable, const char *typeName, int object, int depth);
	bool QueryVariableCustom(lua_State *L, Idx<Variable> variable, const char *typeName, int object, int depth);

	lua_State *QueryParentThread(lua_State *L);

private:
	bool QueryVariableGeneric(lua_State *L, Idx<Variable> variable, const char *typeName, int object, int depth, const char* queryFunction);
};
