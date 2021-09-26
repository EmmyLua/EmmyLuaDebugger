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
#include <cstddef>
//luacfg
typedef double lua_Number;
// 不同的lua版本定义是不同的，不同的平台定义也是不同的
// x64 上最终都是long long
// x86 上lua5.3以上是long long，lua5.2以下是ptrdiff_t通常是int
// 传参时
typedef long long lua_Integer;
typedef ptrdiff_t lua_KContext;

#include "emmy_debugger/api/lua_state.h"

/*
@@ LUA_IDSIZE gives the maximum size for the description of the source
@@ of a function in debug information.
** CHANGE it if you want a different size.
*/
#define LUA_IDSIZE	1024
/* option for multiple returns in 'lua_pcall' and 'lua_call' */
#define LUA_MULTRET	(-1)

/* thread status */
#define LUA_OK		0
#define LUA_YIELD	1
#define LUA_ERRRUN	2
#define LUA_ERRSYNTAX	3
#define LUA_ERRMEM	4
#define LUA_ERRGCMM	5
#define LUA_ERRERR	6


// typedef struct lua_State lua_State;


/*
** basic types
*/
#define LUA_TNONE		(-1)
#define LUA_TNIL		0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER		3
#define LUA_TSTRING		4
#define LUA_TTABLE		5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD		8

/*
** Type for C functions registered with Lua
*/
typedef int(*lua_CFunction) (lua_State *L);

/*
** Type for continuation functions
*/
typedef int(*lua_KFunction) (lua_State *L, int status, lua_KContext ctx);


/*
** Type for functions that read/write blocks when loading/dumping Lua chunks
*/
typedef const char * (*lua_Reader) (lua_State *L, void *ud, size_t *sz);

typedef int(*lua_Writer) (lua_State *L, const void *p, size_t sz, void *ud);


/*
** Type for memory-allocation functions
*/
typedef void * (*lua_Alloc) (void *ud, void *ptr, size_t osize, size_t nsize);

//debug
/*
** Event codes
*/
#define LUA_HOOKCALL	0
#define LUA_HOOKRET	1
#define LUA_HOOKLINE	2
#define LUA_HOOKCOUNT	3
// LUA 5.1
#define LUA_HOOKTAILRET 4
// LUA 5.2+
#define LUA_HOOKTAILCALL 4


/*
** Event masks
*/
#define LUA_MASKCALL	(1 << LUA_HOOKCALL)
#define LUA_MASKRET		(1 << LUA_HOOKRET)
#define LUA_MASKLINE	(1 << LUA_HOOKLINE)
#define LUA_MASKCOUNT	(1 << LUA_HOOKCOUNT)

struct lua_Debug_51 {
	int event;
	const char *name;	/* (n) */
	const char *namewhat;	/* (n) `global', `local', `field', `method' */
	const char *what;	/* (S) `Lua', `C', `main', `tail' */
	const char *source;	/* (S) */
	int currentline;	/* (l) */
	int nups;		/* (u) number of upvalues */
	int linedefined;	/* (S) */
	int lastlinedefined;	/* (S) */
	char short_src[LUA_IDSIZE]; /* (S) */
	/* private part */
	int i_ci;  /* active function */
};

struct lua_Debug_52 {
	int event;
	const char *name;	/* (n) */
	const char *namewhat;	/* (n) 'global', 'local', 'field', 'method' */
	const char *what;	/* (S) 'Lua', 'C', 'main', 'tail' */
	const char *source;	/* (S) */
	int currentline;	/* (l) */
	int linedefined;	/* (S) */
	int lastlinedefined;	/* (S) */
	unsigned char nups;	/* (u) number of upvalues */
	unsigned char nparams;/* (u) number of parameters */
	char isvararg;        /* (u) */
	char istailcall;	/* (t) */
	char short_src[LUA_IDSIZE]; /* (S) */
	/* private part */
	struct CallInfo *i_ci;  /* active function */
};

struct lua_Debug_53 {
	int event;
	const char *name;	/* (n) */
	const char *namewhat;	/* (n) 'global', 'local', 'field', 'method' */
	const char *what;	/* (S) 'Lua', 'C', 'main', 'tail' */
	const char *source;	/* (S) */
	int currentline;	/* (l) */
	int linedefined;	/* (S) */
	int lastlinedefined;	/* (S) */
	unsigned char nups;	/* (u) number of upvalues */
	unsigned char nparams;/* (u) number of parameters */
	char isvararg;        /* (u) */
	char istailcall;	/* (t) */
	char short_src[LUA_IDSIZE]; /* (S) */
	/* private part */
	struct CallInfo *i_ci;  /* active function */
};

struct lua_Debug_54 {
	int event;
	const char* name;	/* (n) */
	const char* namewhat;	/* (n) 'global', 'local', 'field', 'method' */
	const char* what;	/* (S) 'Lua', 'C', 'main', 'tail' */
	const char* source;	/* (S) */
	size_t srclen;	/* (S) */
	int currentline;	/* (l) */
	int linedefined;	/* (S) */
	int lastlinedefined;	/* (S) */
	unsigned char nups;	/* (u) number of upvalues */
	unsigned char nparams;/* (u) number of parameters */
	char isvararg;        /* (u) */
	char istailcall;	/* (t) */
	unsigned short ftransfer;   /* (r) index of first value transferred */
	unsigned short ntransfer;   /* (r) number of transferred values */
	char short_src[LUA_IDSIZE]; /* (S) */
	/* private part */
	struct CallInfo* i_ci;  /* active function */
};

struct lua_Debug {
	union {
		lua_Debug_51 ar51;
		lua_Debug_52 ar52;
		lua_Debug_53 ar53;
		lua_Debug_54 ar54;
	} u;
};

int getDebugEvent(lua_Debug* ar);
int getDebugCurrentLine(lua_Debug* ar);
int getDebugLineDefined(lua_Debug* ar);
const char* getDebugSource(lua_Debug* ar);
const char* getDebugName(lua_Debug* ar);



/* Functions to be called by the debugger in specific events */
typedef void(*lua_Hook) (lua_State *L, lua_Debug *ar);

//#include "lauxlib.h"
typedef struct luaL_Reg {
	const char *name;
	lua_CFunction func;
} luaL_Reg;

#define luaL_checkstring(L,n)	(luaL_checklstring(L, (n), NULL))

#define luaL_newlibtable(L,l)	\
  lua_createtable(L, 0, sizeof(l)/sizeof((l)[0]) - 1)

#define luaL_newlib(L,l)  \
  (luaL_newlibtable(L,l), luaL_setfuncs(L,l,0))

#define lua_tostring(L,i)	lua_tolstring(L, (i), NULL)
#define lua_pop(L,n)		lua_settop(L, -(n)-1)
#define lua_newtable(L)		lua_createtable(L, 0, 0)
#define lua_register(L,n,f) (lua_pushcfunction(L, (f)), lua_setglobal(L, (n)))
#define lua_pushcfunction(L,f)	lua_pushcclosure(L, (f), 0)
#define lua_isfunction(L,n)	(lua_type(L, (n)) == LUA_TFUNCTION)
#define lua_istable(L,n)	(lua_type(L, (n)) == LUA_TTABLE)
#define lua_islightuserdata(L,n)	(lua_type(L, (n)) == LUA_TLIGHTUSERDATA)
#define lua_isnil(L,n)		(lua_type(L, (n)) == LUA_TNIL)
#define lua_isboolean(L,n)	(lua_type(L, (n)) == LUA_TBOOLEAN)
#define lua_isthread(L,n)	(lua_type(L, (n)) == LUA_TTHREAD)
#define lua_isnone(L,n)		(lua_type(L, (n)) == LUA_TNONE)
#define lua_isnoneornil(L, n)	(lua_type(L, (n)) <= 0)
////////////////////////////////////////////////////////////////////////////////////////
extern int LUA_REGISTRYINDEX;


#if defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && defined(__ELF__)
#define FUNC __attribute__((visibility("hidden")))
#else
#define FUNC
#endif

#define DEF_LUA_API(FN) extern dll_##FN FN
#define IMP_LUA_API(FN) FUNC dll_##FN FN
#define LOAD_LUA_API(FN) FN = (dll_##FN) LoadAPI(""#FN""); if (FN == nullptr) return false

#define DEF_LUA_API_E(FN) extern dll_e_##FN e_##FN
#define IMP_LUA_API_E(FN) FUNC dll_e_##FN e_##FN
#define LOAD_LUA_API_E(FN) e_##FN = (dll_e_##FN) LoadAPI(""#FN"")

typedef int (*dll_lua_gettop)(lua_State* L);
DEF_LUA_API(lua_gettop);
typedef void (*dll_lua_settop)(lua_State* L, int idx);
DEF_LUA_API(lua_settop);
typedef int (*dll_lua_type)(lua_State* L, int idx);
DEF_LUA_API(lua_type);
typedef const char*(*dll_lua_typename)(lua_State* L, int tp);
DEF_LUA_API(lua_typename);
typedef const char*(*dll_lua_tolstring)(lua_State* L, int idx, size_t* len);
DEF_LUA_API(lua_tolstring);
typedef int (*dll_lua_toboolean)(lua_State* L, int idx);
DEF_LUA_API(lua_toboolean);
typedef void (*dll_lua_pushnil)(lua_State* L);
DEF_LUA_API(lua_pushnil);
typedef void (*dll_lua_pushnumber)(lua_State* L, lua_Number n);
DEF_LUA_API(lua_pushnumber);
typedef const char*(*dll_lua_pushlstring)(lua_State* L, const char* s, size_t len);
DEF_LUA_API(lua_pushlstring);
typedef const char*(*dll_lua_pushstring)(lua_State* L, const char* s);
DEF_LUA_API(lua_pushstring);
typedef void (*dll_lua_pushcclosure)(lua_State* L, lua_CFunction fn, int n);
DEF_LUA_API(lua_pushcclosure);
typedef void (*dll_lua_pushboolean)(lua_State* L, int b);
DEF_LUA_API(lua_pushboolean);
typedef void (*dll_lua_pushvalue)(lua_State* L, int idx);
DEF_LUA_API(lua_pushvalue);
typedef int (*dll_lua_getfield)(lua_State* L, int idx, const char* k);
DEF_LUA_API(lua_getfield);
typedef int (*dll_lua_next)(lua_State* L, int idx);
DEF_LUA_API(lua_next);
typedef void (*dll_lua_createtable)(lua_State* L, int narr, int nrec);
DEF_LUA_API(lua_createtable);
typedef void (*dll_lua_setfield)(lua_State* L, int idx, const char* k);
DEF_LUA_API(lua_setfield);
typedef int (*dll_lua_setmetatable)(lua_State* L, int objindex);
DEF_LUA_API(lua_setmetatable);
typedef int (*dll_lua_getstack)(lua_State* L, int level, lua_Debug* ar);
DEF_LUA_API(lua_getstack);
typedef int (*dll_lua_getinfo)(lua_State* L, const char* what, lua_Debug* ar);
DEF_LUA_API(lua_getinfo);
typedef const char*(*dll_lua_getlocal)(lua_State* L, const lua_Debug* ar, int n);
DEF_LUA_API(lua_getlocal);
typedef const char*(*dll_lua_getupvalue)(lua_State* L, int funcindex, int n);
DEF_LUA_API(lua_getupvalue);
typedef const char*(*dll_lua_setupvalue)(lua_State* L, int funcindex, int n);
DEF_LUA_API(lua_setupvalue);
typedef void (*dll_lua_sethook)(lua_State* L, lua_Hook func, int mask, int count);
DEF_LUA_API(lua_sethook);
typedef int (*dll_luaL_loadstring)(lua_State* L, const char* s);
DEF_LUA_API(luaL_loadstring);
typedef const char*(*dll_luaL_checklstring)(lua_State* L, int arg, size_t* l);
DEF_LUA_API(luaL_checklstring);
typedef lua_Number (*dll_luaL_checknumber)(lua_State* L, int arg);
DEF_LUA_API(luaL_checknumber);
typedef void*(*dll_lua_topointer)(lua_State* L, int index);
DEF_LUA_API(lua_topointer);
typedef int(*dll_lua_getmetatable)(lua_State *L, int objindex);
DEF_LUA_API(lua_getmetatable);
typedef int(*dll_lua_rawget)(lua_State *L, int idx);
DEF_LUA_API(lua_rawget);
typedef void(*dll_lua_rawset)(lua_State *L, int idx);
DEF_LUA_API(lua_rawset);
typedef void(*dll_lua_pushlightuserdata)(lua_State *L, void *p);
DEF_LUA_API(lua_pushlightuserdata);
typedef void*(*dll_lua_touserdata)(lua_State *L, int idx);
DEF_LUA_API(lua_touserdata);
typedef void*(*dll_lua_rawseti)(lua_State *L, int idx, lua_Integer n);
DEF_LUA_API(lua_rawseti);
typedef void*(*dll_lua_rawgeti)(lua_State *L, int idx, lua_Integer n);
DEF_LUA_API(lua_rawgeti);
typedef void* (*dll_luaL_newstate)(void);
DEF_LUA_API(luaL_newstate);
typedef void (*dll_lua_close)(lua_State* L);
DEF_LUA_API(lua_close);
typedef int (*dll_lua_pushthread)(lua_State* L);
DEF_LUA_API(lua_pushthread);

//51
typedef int (*dll_e_lua_setfenv)(lua_State* L, int idx);
DEF_LUA_API_E(lua_setfenv);
typedef lua_Integer (*dll_e_lua_tointeger)(lua_State* L, int idx);
DEF_LUA_API_E(lua_tointeger);
typedef lua_Number(*dll_e_lua_tonumber) (lua_State *L, int idx);
DEF_LUA_API_E(lua_tonumber);
typedef int (*dll_e_lua_call)(lua_State* L, int nargs, int nresults);
DEF_LUA_API_E(lua_call);
typedef int (*dll_e_lua_pcall)(lua_State* L, int nargs, int nresults, int errfunc);
DEF_LUA_API_E(lua_pcall);
typedef void (*dll_e_lua_insert)(lua_State* L, int idx);
DEF_LUA_API_E(lua_insert);


//51 & 52
typedef void (*dll_e_lua_remove)(lua_State *L, int idx);
DEF_LUA_API_E(lua_remove);
//52 & 53 & 54
typedef lua_Integer (*dll_e_lua_tointegerx)(lua_State* L, int idx, int* isnum);
DEF_LUA_API_E(lua_tointegerx);
typedef lua_Number(*dll_e_lua_tonumberx) (lua_State *L, int idx, int *isnum);
DEF_LUA_API_E(lua_tonumberx);
typedef int (*dll_e_lua_getglobal)(lua_State* L, const char* name);
DEF_LUA_API_E(lua_getglobal);
typedef void (*dll_e_lua_setglobal)(lua_State* L, const char* name);
DEF_LUA_API_E(lua_setglobal);
typedef void (*dll_e_lua_callk)(lua_State* L, int nargs, int nresults, lua_KContext ctx, lua_KFunction k);
DEF_LUA_API_E(lua_callk);
typedef int (*dll_e_lua_pcallk)(lua_State* L, int nargs, int nresults, int errfunc, lua_KContext ctx, lua_KFunction k);
DEF_LUA_API_E(lua_pcallk);
typedef void (*dll_e_luaL_setfuncs)(lua_State* L, const luaL_Reg* l, int nup);
DEF_LUA_API_E(luaL_setfuncs);
typedef int(*dll_e_lua_absindex)(lua_State *L, int idx);
DEF_LUA_API_E(lua_absindex);
typedef int(*dll_e_lua_rawgetp)(lua_State *L, int idx, const void* p);
DEF_LUA_API_E(lua_rawgetp);
typedef void(*dll_e_lua_rawsetp)(lua_State* L, int idx, const void* p);
DEF_LUA_API_E(lua_rawsetp);
typedef int(*dll_e_lua_pushthread)(lua_State* L);
DEF_LUA_API_E(lua_pushthread);

// 51 & 52 & 53
typedef void* (*dll_e_lua_newuserdata)(lua_State* L, int size);
DEF_LUA_API_E(lua_newuserdata);
//53
typedef void (*dll_e_lua_rotate)(lua_State *L, int idx, int n);
DEF_LUA_API_E(lua_rotate);

//54
typedef void* (*dll_e_lua_newuserdatauv)(lua_State* L, int size, int nuvalue);
DEF_LUA_API_E(lua_newuserdatauv);

//jit
typedef int (*dll_e_luaopen_jit)(lua_State* L);
DEF_LUA_API_E(luaopen_jit);

lua_Integer lua_tointeger(lua_State* L, int idx);
lua_Number lua_tonumber(lua_State* L, int idx);
int lua_setfenv(lua_State* L, int idx);
int lua_getglobal(lua_State* L, const char* name);
void lua_setglobal(lua_State* L, const char* name);
int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc);
int lua_upvalueindex(int i);
int lua_absindex(lua_State *L, int idx);
void lua_call(lua_State* L, int nargs, int nresults);
void luaL_setfuncs(lua_State* L, const luaL_Reg* l, int nup);
void lua_remove(lua_State *L, int idx);
void* lua_newuserdata(lua_State* L, int size);
void lua_pushglobaltable(lua_State* L);
int lua_rawgetp(lua_State* L, int idx, const void* p);
void lua_rawsetp(lua_State* L, int idx, const void* p);
void lua_insert(lua_State* L, int idx);
