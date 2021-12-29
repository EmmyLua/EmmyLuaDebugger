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

#include <rapidjson/document.h>
#include <mutex>
#include <condition_variable>
#include <map>
#include "emmy_debugger/transporter.h"
#include "emmy_debugger/api/lua_api.h"
#include "emmy_debugger/emmy_debugger_manager.h"

enum class LogType
{
	Info,
	Warning,
	Error
};

enum class WorkMode
{
	EmmyCore,
	Attach,
};

class EmmyFacade
{
public:
	static EmmyFacade& Get();

	static void HookLua(lua_State* L, lua_Debug* ar);

	static void ReadyLuaHook(lua_State* L, lua_Debug* ar);

	EmmyFacade();
	~EmmyFacade();
#ifndef EMMY_USE_LUA_SOURCE
	bool SetupLuaAPI();
#endif

	bool TcpListen(lua_State* L, const std::string& host, int port, std::string& err);
	bool TcpSharedListen(lua_State* L, const std::string& host, int port, std::string& err);
	bool TcpConnect(lua_State* L, const std::string& host, int port, std::string& err);
	bool PipeListen(lua_State* L, const std::string& name, std::string& err);
	bool PipeConnect(lua_State* L, const std::string& name, std::string& err);
	int BreakHere(lua_State* L);
	
	int OnConnect(bool suc);
	int OnDisconnect();
	void WaitIDE(bool force = false, int timeout = 0);
	void OnReceiveMessage(const rapidjson::Document& document);
	bool OnBreak(std::shared_ptr<Debugger> debugger);
	void Destroy();
	void OnEvalResult(std::shared_ptr<EvalContext> context);
	void SendLog(LogType type, const char* fmt, ...);
	void OnLuaStateGC(lua_State* L);
	void Hook(lua_State* L, lua_Debug* ar);
	std::shared_ptr<EmmyDebuggerManager> GetDebugManager() const;

	std::shared_ptr<Variable> GetVariableRef(Variable* variable);
	void AddVariableRef(std::shared_ptr<Variable> variable);
	void RemoveVariableRef(Variable* variable);
	std::shared_ptr<Debugger> GetDebugger(lua_State* L);

	void SetReadyHook(lua_State* L);

	void StartDebug();

	// for hook
	void StartupHookMode(int port);
	void Attach(lua_State* L);

	void SetWorkMode(WorkMode mode);
	WorkMode GetWorkMode();
	// Start hook 作为成员存在
	std::function<void()> StartHook;

private:
	void OnInitReq(const rapidjson::Document& document);
	void OnReadyReq(const rapidjson::Document& document);
	void OnAddBreakPointReq(const rapidjson::Document& document);
	void OnRemoveBreakPointReq(const rapidjson::Document& document);
	void OnActionReq(const rapidjson::Document& document);
	void OnEvalReq(const rapidjson::Document& document);

	std::mutex waitIDEMutex;
	std::condition_variable waitIDECV;
	
	std::shared_ptr<Transporter> transporter;
	std::map<Variable*, std::shared_ptr<Variable>> luaVariableRef;
	
	bool isIDEReady;
	bool isAPIReady;
	bool isWaitingForIDE;
	WorkMode workMode;

	std::shared_ptr<EmmyDebuggerManager> emmyDebuggerManager;

	// 表示使用了tcplisten tcpConnect 的states
	std::set<lua_State*> mainStates;

	std::mutex readyHookMtx;
	bool readyHook;
};




