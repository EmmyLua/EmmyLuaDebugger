#pragma once

#include "emmy_debugger/api/lua_api.h"
#include "emmy_debugger/proto/proto.h"

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

// emmy.registerTypeName(typeName: string): bool
int registerTypeName(lua_State* L);

bool install_emmy_debugger(struct lua_State* L);

std::string prepareEvalExpr(const std::string& eval);

