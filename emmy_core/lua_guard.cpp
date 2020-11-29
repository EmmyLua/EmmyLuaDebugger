#include "lua_guard.h"

LuaGlobalIgnoreMetatable::LuaGlobalIgnoreMetatable(lua_State* L)
	: _L(L), _idx(-1), _metaIdx(-1), _top(0)
{
	_top = lua_gettop(L);
	// ȫ�ֱ���ѹջ
	lua_getglobal(L, "_G");
	_idx = lua_gettop(L);
	int ret = lua_getmetatable(L, _idx);
	// ����Ԫ��
	if (ret == 1) {
		_metaIdx = lua_gettop(L);// ��ʱ_G��Ԫ����ջ��Ԫ��
		lua_pushnil(L);
		lua_setmetatable(L, _idx);
		// ��ʱջ������ ֮ǰ��Ԫ��
	}
	
}

LuaGlobalIgnoreMetatable ::~LuaGlobalIgnoreMetatable() {
	if (_metaIdx != -1) {
		// ֮�����۸���ɶ,ȫ���
		lua_settop(_L, _metaIdx);
		// set metatable �ᵯ��ջ��Ԫ��
		lua_setmetatable(_L, _idx);
		
	}
	//��ʱջ�����ֺ�guard ֮ǰһ��
	lua_settop(_L, _top);
}
