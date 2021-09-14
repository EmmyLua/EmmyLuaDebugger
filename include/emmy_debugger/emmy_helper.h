#pragma once

#include "api/lua_api.h"
#include "types.h"

bool query_variable(lua_State* L, std::shared_ptr<Variable> variable, const char* typeName, int object, int depth);

// emmy.tcpListen(host: string, port: int): bool
int tcpListen(struct lua_State* L);

// emmy.tcpConnect(host: string, port: int): bool
int tcpConnect(lua_State* L);

// emmy.pipeListen(pipeName: string): bool
int pipeListen(lua_State* L);

// emmy.pipeConnect(pipeName: string): bool
int pipeConnect(lua_State* L);

// emmy.breakHere(): bool
int breakHere(lua_State* L);

// emmy.waitIDE(timeout: number): void
int waitIDE(lua_State* L);

int tcpSharedListen(lua_State* L);

// emmy.stop()
int stop(lua_State* L);

bool install_emmy_core(struct lua_State* L);



/*
 * @deprecated
 */
void ParsePathParts(const std::string& file, std::vector<std::string>& paths);

// 等于0表示相等
bool CompareIgnoreCase(const std::string& lh, const std::string& rh);

struct CaseInsensitiveLess final
{
	bool operator()(const std::string& lhs, const std::string& rhs) const;
};

// 直到C++ 20才有endwith，这里自己写一个
bool EndWith(const std::string& source, const std::string& end);

std::string BaseName(const std::string& filePath);