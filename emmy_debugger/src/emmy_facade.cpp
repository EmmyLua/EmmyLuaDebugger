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

#include "emmy_debugger/emmy_facade.h"
#include <cstdarg>
#include <cstdint>
#include "nlohmann/json.hpp"
#include "emmy_debugger/transporter/socket_server_transporter.h"
#include "emmy_debugger/transporter/socket_client_transporter.h"
#include "emmy_debugger/transporter/pipeline_server_transporter.h"
#include "emmy_debugger/transporter/pipeline_client_transporter.h"
#include "emmy_debugger/debugger/emmy_debugger.h"
#include "emmy_debugger/debugger/emmy_debugger_lib.h"
#include "emmy_debugger/transporter/transporter.h"
#include "emmy_debugger/api/lua_version.h"

EmmyFacade &EmmyFacade::Get() {
	static EmmyFacade instance;
	return instance;
}

void EmmyFacade::HookLua(lua_State *L, lua_Debug *ar) {
	Get().Hook(L, ar);
}

void EmmyFacade::ReadyLuaHook(lua_State *L, lua_Debug *ar) {
	if (!Get().readyHook) {
		return;
	}
	Get().readyHook = false;

	auto states = FindAllCoroutine(L);

	for (auto state: states) {
		lua_sethook(state, HookLua, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);
	}

	lua_sethook(L, HookLua, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);

	auto debugger = Get().GetDebugger(L);
	if (debugger) {
		debugger->Attach();
	}

	Get().Hook(L, ar);
}

EmmyFacade::EmmyFacade()
	: transporter(nullptr),
	  isIDEReady(false),
	  isAPIReady(false),
	  StartHook(nullptr),
	  isWaitingForIDE(false),
	  workMode(WorkMode::EmmyCore),
	  readyHook(false),
	  _protoHandler(this) {
}

EmmyFacade::~EmmyFacade() {
}

#ifndef EMMY_USE_LUA_SOURCE
extern "C" bool SetupLuaAPI();

bool EmmyFacade::SetupLuaAPI() {
	isAPIReady = ::SetupLuaAPI();
	return isAPIReady;
}

#endif

int LuaError(lua_State *L) {
	std::string msg = lua_tostring(L, 1);
	msg = "[Emmy]" + msg;
	lua_getglobal(L, "error");
	lua_pushstring(L, msg.c_str());
	lua_call(L, 1, 0);
	return 0;
}

bool EmmyFacade::TcpListen(lua_State *L, const std::string &host, int port, std::string &err) {
	Destroy();

	_emmyDebuggerManager.AddDebugger(L);

	SetReadyHook(L);

	const auto s = std::make_shared<SocketServerTransporter>();
	transporter = s;
	// s->SetHandler(shared_from_this());
	const auto suc = s->Listen(host, port, err);
	if (!suc) {
		lua_pushcfunction(L, LuaError);
		lua_pushstring(L, err.c_str());
		lua_call(L, 1, 0);
	}
	return suc;
}

bool EmmyFacade::TcpSharedListen(lua_State *L, const std::string &host, int port, std::string &err) {
	if (transporter == nullptr) {
		return TcpListen(L, host, port, err);
	}
	return true;
}

bool EmmyFacade::TcpConnect(lua_State *L, const std::string &host, int port, std::string &err) {
	Destroy();

	_emmyDebuggerManager.AddDebugger(L);

	SetReadyHook(L);

	const auto c = std::make_shared<SocketClientTransporter>();
	transporter = c;
	// c->SetHandler(shared_from_this());
	const auto suc = c->Connect(host, port, err);
	if (suc) {
		WaitIDE(true);
	} else {
		lua_pushcfunction(L, LuaError);
		lua_pushstring(L, err.c_str());
		lua_call(L, 1, 0);
	}
	return suc;
}

bool EmmyFacade::PipeListen(lua_State *L, const std::string &name, std::string &err) {
	Destroy();

	_emmyDebuggerManager.AddDebugger(L);

	SetReadyHook(L);

	const auto p = std::make_shared<PipelineServerTransporter>();
	transporter = p;
	// p->SetHandler(shared_from_this());
	const auto suc = p->pipe(name, err);
	return suc;
}

bool EmmyFacade::PipeConnect(lua_State *L, const std::string &name, std::string &err) {
	Destroy();

	_emmyDebuggerManager.AddDebugger(L);

	SetReadyHook(L);

	const auto p = std::make_shared<PipelineClientTransporter>();
	transporter = p;
	// p->SetHandler(shared_from_this());
	const auto suc = p->Connect(name, err);
	if (suc) {
		WaitIDE(true);
	}
	return suc;
}

void EmmyFacade::WaitIDE(bool force, int timeout) {
	if (transporter != nullptr
	    && (transporter->IsServerMode() || force)
	    && !isWaitingForIDE
	    && !isIDEReady) {
		isWaitingForIDE = true;
		std::unique_lock<std::mutex> lock(waitIDEMutex);
		if (timeout > 0)
			waitIDECV.wait_for(lock, std::chrono::milliseconds(timeout));
		else
			waitIDECV.wait(lock);
		isWaitingForIDE = false;
	}
}

int EmmyFacade::BreakHere(lua_State *L) {
	if (!isIDEReady)
		return 0;

	_emmyDebuggerManager.HandleBreak(L);

	return 1;
}

int EmmyFacade::OnConnect(bool suc) {
	return 0;
}

int EmmyFacade::OnDisconnect() {
	isIDEReady = false;
	isWaitingForIDE = false;

	_emmyDebuggerManager.OnDisconnect();

	_emmyDebuggerManager.RemoveAllBreakpoints();

	if (workMode == WorkMode::Attach) {
		_emmyDebuggerManager.RemoveAllDebugger();
	}

	return 0;
}

void EmmyFacade::Destroy() {
	OnDisconnect();

	if (transporter) {
		transporter->Stop();
		transporter = nullptr;
	}
}

void EmmyFacade::SetWorkMode(WorkMode mode) {
	workMode = mode;
}

WorkMode EmmyFacade::GetWorkMode() {
	return workMode;
}


void EmmyFacade::InitReq(InitParams & params) {
	if (StartHook) {
		StartHook();
	}

	_emmyDebuggerManager.helperCode = params.emmyHelper;
	_emmyDebuggerManager.extNames.clear();
	_emmyDebuggerManager.extNames = params.ext;

	// 这里有个线程安全问题，消息线程和lua 执行线程不是相同线程，但是没有一个锁能让我做同步
	// 所以我不能在这里访问lua state 指针的内部结构
	//
	// 方案：提前为主state 设置hook 利用hook 实现同步

	// fix 以上安全问题
	StartDebug();
}

void EmmyFacade::ReadyReq() {
	isIDEReady = true;
	waitIDECV.notify_all();
}

void EmmyFacade::OnReceiveMessage(nlohmann::json document) {
	_protoHandler.OnDispatch(document);
}

bool EmmyFacade::OnBreak(std::shared_ptr<Debugger> debugger) {
	if (!debugger) {
		return false;
	}
	std::vector<Stack> stacks;

	_emmyDebuggerManager.SetHitDebugger(debugger);

	debugger->GetStacks(stacks);

	auto obj = nlohmann::json::object();
	obj["cmd"] = static_cast<int>(MessageCMD::BreakNotify);
	obj["stacks"] = JsonProtocol::SerializeArray(stacks);

	transporter->Send(int(MessageCMD::BreakNotify), obj);

	return true;
}

void EmmyFacade::OnEvalResult(std::shared_ptr<EvalContext> context) {
	if (transporter) {
		transporter->Send(int(MessageCMD::EvalRsp), context->Serialize());
	}
}

void EmmyFacade::SendLog(LogType type, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	char buff[1024] = {0};
	vsnprintf(buff, 1024, fmt, args);
	va_end(args);

	const std::string msg = buff;

	auto obj = nlohmann::json::object();
	obj["type"] = type;
	obj["message"] = msg;

	if (transporter) {
		transporter->Send(int(MessageCMD::LogNotify), obj);
	}
}

void EmmyFacade::OnLuaStateGC(lua_State *L) {
	auto debugger = _emmyDebuggerManager.RemoveDebugger(L);

	if (debugger) {
		debugger->Detach();
	}

	if (workMode == WorkMode::EmmyCore) {
		if (_emmyDebuggerManager.IsDebuggerEmpty()) {
			Destroy();
		}
	}
}

void EmmyFacade::Hook(lua_State *L, lua_Debug *ar) {
	auto debugger = GetDebugger(L);
	if (debugger) {
		if (!debugger->IsRunning()) {
			if (GetWorkMode() == WorkMode::EmmyCore) {
				if (luaVersion != LuaVersion::LUA_JIT) {
					if (debugger->IsMainCoroutine(L)) {
						SetReadyHook(L);
					}
				} else {
					SetReadyHook(L);
				}
			}
			return;
		}

		debugger->Hook(ar, L);
	} else {
		if (workMode == WorkMode::Attach) {
			debugger = _emmyDebuggerManager.AddDebugger(L);
			install_emmy_debugger(L);
			if (_emmyDebuggerManager.IsRunning()) {
				debugger->Start();
				debugger->Attach();
			}
			// send attached notify
			auto obj = nlohmann::json::object();
			obj["state"] = reinterpret_cast<int64_t>(L);

			this->transporter->Send(int(MessageCMD::AttachedNotify), obj);

			debugger->Hook(ar, L);
		}
	}
}

EmmyDebuggerManager &EmmyFacade::GetDebugManager() {
	return _emmyDebuggerManager;
}

std::shared_ptr<Debugger> EmmyFacade::GetDebugger(lua_State *L) {
	return _emmyDebuggerManager.GetDebugger(L);
}

void EmmyFacade::SetReadyHook(lua_State *L) {
	lua_sethook(L, ReadyLuaHook, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);
}

void EmmyFacade::StartDebug() {
	_emmyDebuggerManager.SetRunning(true);
	readyHook = true;
}

void EmmyFacade::StartupHookMode(int port) {
	Destroy();

	// 1024 - 65535
	while (port > 0xffff) port -= 0xffff;
	while (port < 0x400) port += 0x400;

	const auto s = std::make_shared<SocketServerTransporter>();
	std::string err;
	const auto suc = s->Listen("localhost", port, err);
	if (suc) {
		transporter = s;
		// transporter->SetHandler(shared_from_this());
	}
}

void EmmyFacade::Attach(lua_State *L) {
	if (!this->transporter->IsConnected())
		return;

	// 这里存在一个问题就是 hook 的时机太早了，globalstate 都还没初始化完毕

	if (!isAPIReady) {
		// 考虑到emmy_hook use lua source
		isAPIReady = install_emmy_debugger(L);
	}

	lua_sethook(L, EmmyFacade::HookLua, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);
}

bool EmmyFacade::RegisterTypeName(lua_State *L, const std::string &typeName, std::string &err) {
	auto debugger = GetDebugger(L);
	if (!debugger) {
		err = "Debugger does not exist";
		return false;
	}
	const auto suc = debugger->RegisterTypeName(typeName, err);
	return suc;
}
