#include "emmy_debugger/extension_point.h"
#include "emmy_debugger/emmy_facade.h"

std::string ExtensionPoint::ExtensionTable = "emmyHelper";

int metaQuery(lua_State *L) {
	const int argN = lua_gettop(L);
	auto *var = (Variable *) lua_touserdata(L, 1);
	const int index = 2;
	const auto depth = lua_tonumber(L, 3);
	bool queryHelper = false;
	if (argN >= 4) {
		queryHelper = lua_toboolean(L, 4);
	}
	auto debugger = EmmyFacade::Get().GetDebugger(L);
	auto variable = EmmyFacade::Get().GetVariableRef(var);

	if (debugger && variable) {
		debugger->GetVariable(L, variable, index, static_cast<int>(depth), queryHelper);
	}
	return 0;
}

int metaAddChild(lua_State *L) {
	auto *var = (Variable *) lua_touserdata(L, 1);
	auto *child = (Variable *) lua_touserdata(L, 2);
	auto childRef = EmmyFacade::Get().GetVariableRef(child);
	if (childRef) {
		var->children.push_back(childRef);
	}

	return 0;
}

int metaIndex(lua_State *L) {
	const Variable *var = (Variable *) lua_touserdata(L, 1);
	const std::string k = lua_tostring(L, 2);
	if (k == "name") {
		lua_pushstring(L, var->name.c_str());
	} else if (k == "value") {
		lua_pushstring(L, var->value.c_str());
	} else if (k == "valueType") {
		lua_pushnumber(L, var->valueType);
	} else if (k == "valueTypeName") {
		lua_pushstring(L, var->valueTypeName.c_str());
	} else if (k == "addChild") {
		lua_pushcfunction(L, metaAddChild);
	} else if (k == "query") {
		lua_pushcfunction(L, metaQuery);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int metaNewIndex(lua_State *L) {
	auto var = (Variable *) lua_touserdata(L, 1);
	const std::string k = lua_tostring(L, 2);
	if (k == "name") {
		const char *value = lua_tostring(L, 3);
		var->name = value ? value : "";
	} else if (k == "value") {
		const char *value = lua_tostring(L, 3);
		var->value = value ? value : "";
	} else if (k == "valueType") {
		var->valueType = static_cast<int>(lua_tonumber(L, 3));
	} else if (k == "valueTypeName") {
		const char *value = lua_tostring(L, 3);
		var->valueTypeName = value ? value : "";
	}
	return 0;
}

int metaGC(lua_State *L) {
	const auto var = (Variable *) lua_touserdata(L, 1);
	EmmyFacade::Get().RemoveVariableRef(var);

	return 0;
}

void setupMeta(lua_State *L) {
	lua_newtable(L);

	lua_pushcfunction(L, metaIndex);
	lua_setfield(L, -2, "__index");

	lua_pushcfunction(L, metaNewIndex);
	lua_setfield(L, -2, "__newindex");

	lua_pushcfunction(L, metaGC);
	lua_setfield(L, -2, "__gc");

	lua_setmetatable(L, -2);
}

int createNode(lua_State *L) {
	auto variable = std::make_shared<Variable>();
	EmmyFacade::Get().AddVariableRef(variable);
	lua_pushlightuserdata(L, variable.get());
	setupMeta(L);
	return 1;
}

ExtensionPoint::ExtensionPoint() {
}

// emmy_init(emmy)
int emmyHelperInit(lua_State *L) {
	lua_getglobal(L, ExtensionPoint::ExtensionTable.c_str());
	if (lua_istable(L, -1)) {
		// create node
		lua_pushcfunction(L, createNode);
		lua_setfield(L, -2, "createNode");
	}
	return 0;
}

void ExtensionPoint::Initialize(lua_State *L) {
	lua_pushglobaltable(L);
	lua_pushstring(L, "emmyHelperInit");
	lua_pushcfunction(L, emmyHelperInit);
	lua_rawset(L, -3);
	lua_pop(L, 1);
}

bool ExtensionPoint::QueryVariable(lua_State *L, std::shared_ptr<Variable> variable, const char *typeName, int object, int depth) {
	bool result = false;
	object = lua_absindex(L, object);
	const int t = lua_gettop(L);
	lua_getglobal(L, ExtensionTable.c_str());

	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, "queryVariable");
		if (lua_isfunction(L, -1)) {
			EmmyFacade::Get().AddVariableRef(variable);

			lua_pushlightuserdata(L, variable.get());
			setupMeta(L);
			lua_pushvalue(L, object);
			lua_pushstring(L, typeName);
			lua_pushnumber(L, depth);
			const auto r = lua_pcall(L, 4, 1, 0);
			if (r == LUA_OK) {
				result = lua_toboolean(L, -1);
			} else {
				const auto err = lua_tostring(L, -1);
				printf("query error: %s\n", err);
			}
		}
	}

	lua_settop(L, t);
	return result;
}

lua_State *ExtensionPoint::QueryParentThread(lua_State *L) {
	lua_State *PL = nullptr;
	const int t = lua_gettop(L);
	lua_getglobal(L, ExtensionTable.c_str());
	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, "queryParentThread");
		if (lua_isfunction(L, -1)) {
			const auto r = lua_pcall(L, 0, 1, 0);
			if (r == LUA_OK) {
				PL = lua_tothread(L, -1);
			}
		}
	}
	lua_settop(L, t);

	return PL;
}
