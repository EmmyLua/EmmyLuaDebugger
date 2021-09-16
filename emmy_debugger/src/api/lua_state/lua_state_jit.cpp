#include "emmy_debugger/api/lua_state.h"

// luajit不包含luajit头文件，所以本质上getmainstate 结果是错的
// 但是luajit有global hook，所以不影响调试

lua_State* GetMainState_luaJIT(lua_State* L)
{
	return L;
}
