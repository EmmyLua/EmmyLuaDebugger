#pragma once

#include <string>
#include "api/lua_api.h"
#include "types.h"

class ExtensionPoint {
public:
	static std::string ExtensionTable;

	ExtensionPoint();

	void Initialize(lua_State* L);
	// 
	bool QueryVariable(lua_State *L, std::shared_ptr<Variable> variable, const char *typeName, int object, int depth);

	lua_State *QueryParentThread(lua_State *L);

};
