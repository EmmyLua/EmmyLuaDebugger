
typedef struct lua_State lua_State;

typedef unsigned char lu_type_general;

#define CommonHeaderGeneral	GCObjectGeneral *next; lu_type_general tt; lu_type_general marked

struct GCObjectGeneral
{
	CommonHeaderGeneral;
};

GCObjectGeneral* GetGCHead(lua_State* L);

GCObjectGeneral* GetGCHead_lua54(lua_State* L);

GCObjectGeneral* GetGCHead_lua53(lua_State* L);

GCObjectGeneral* GetGCHead_lua52(lua_State* L);

GCObjectGeneral* GetGCHead_lua51(lua_State* L);

lua_State* GetMainState(lua_State* L);

lua_State* GetMainState_lua54(lua_State* L);

lua_State* GetMainState_lua53(lua_State* L);

lua_State* GetMainState_lua52(lua_State* L);

lua_State* GetMainState_lua51(lua_State* L);