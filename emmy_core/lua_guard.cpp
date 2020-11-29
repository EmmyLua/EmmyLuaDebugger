#include "lua_guard.h"

LuaGlobalIgnoreMetatable::LuaGlobalIgnoreMetatable(lua_State* L)
	: _L(L), _idx(-1), _metaIdx(-1), _top(0)
{
	_top = lua_gettop(L);
	// 全局变量压栈
	lua_getglobal(L, "_G");
	_idx = lua_gettop(L);
	int ret = lua_getmetatable(L, _idx);
	// 若有元表
	if (ret == 1) {
		_metaIdx = lua_gettop(L);// 此时_G的元表是栈顶元素
		lua_pushnil(L);
		lua_setmetatable(L, _idx);
		// 此时栈顶就是 之前的元表
	}
	
}

LuaGlobalIgnoreMetatable ::~LuaGlobalIgnoreMetatable() {
	if (_metaIdx != -1) {
		// 之上无论干了啥,全清掉
		lua_settop(_L, _metaIdx);
		// set metatable 会弹出栈顶元素
		lua_setmetatable(_L, _idx);
		
	}
	//此时栈顶保持和guard 之前一样
	lua_settop(_L, _top);
}
