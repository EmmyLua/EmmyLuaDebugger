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
#include "emmy_debugger/proto/socket_server_transporter.h"
#include "emmy_debugger/proto/socket_client_transporter.h"
#include "emmy_debugger/proto/pipeline_server_transporter.h"
#include "emmy_debugger/proto/pipeline_client_transporter.h"
#include "emmy_debugger/emmy_debugger.h"
#include "emmy_debugger/transporter.h"
#include "emmy_debugger/emmy_helper.h"
#include "emmy_debugger/lua_version.h"

EmmyFacade& EmmyFacade::Get()
{
	static EmmyFacade instance;
	return instance;
}

void EmmyFacade::HookLua(lua_State* L, lua_Debug* ar)
{
	EmmyFacade::Get().Hook(L, ar);
}

void EmmyFacade::InitHook(lua_State* L, lua_Debug* ar)
{
	auto mainL = GetMainState(L);

	auto states = FindAllCoroutine(mainL);

	for (auto state : states)
	{
		lua_sethook(state, HookLua, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);
	}

	lua_sethook(L, HookLua, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);

	EmmyFacade::Get().Hook(L, ar);
}

EmmyFacade::EmmyFacade()
	: transporter(nullptr),
	  isIDEReady(false),
	  isAPIReady(false),
	  isWaitingForIDE(false),
	  StartHook(nullptr),
	  workMode(WorkMode::EmmyCore),
	  emmyDebuggerManager(std::make_shared<EmmyDebuggerManager>())
{
}

EmmyFacade::~EmmyFacade()
{
}

#ifndef EMMY_USE_LUA_SOURCE
extern "C" bool SetupLuaAPI();

bool EmmyFacade::SetupLuaAPI()
{
	isAPIReady = ::SetupLuaAPI();
	return isAPIReady;
}

#endif

int LuaError(lua_State* L)
{
	std::string msg = lua_tostring(L, 1);
	msg = "[Emmy]" + msg;
	lua_getglobal(L, "error");
	lua_pushstring(L, msg.c_str());
	lua_call(L, 1, 0);
	return 0;
}

int LuaPrint(lua_State* L)
{
	std::string msg = lua_tostring(L, 1);
	msg = "[Emmy]" + msg;
	lua_getglobal(L, "print");
	lua_pushstring(L, msg.c_str());
	lua_call(L, 1, 0);
	return 0;
}

bool EmmyFacade::TcpListen(lua_State* L, const std::string& host, int port, std::string& err)
{
	Destroy();

	// 仅仅luajit需要
	mainStates.insert(L);

	emmyDebuggerManager->AddDebugger(L);

	const auto s = std::make_shared<SocketServerTransporter>();
	transporter = s;
	// s->SetHandler(shared_from_this());
	const auto suc = s->Listen(host, port, err);
	if (!suc)
	{
		lua_pushcfunction(L, LuaError);
		lua_pushstring(L, err.c_str());
		lua_call(L, 1, 0);
	}
	return suc;
}

bool EmmyFacade::TcpSharedListen(lua_State* L, const std::string& host, int port, std::string& err)
{
	if (transporter == nullptr)
	{
		return TcpListen(L, host, port, err);
	}
	return true;
}

bool EmmyFacade::TcpConnect(lua_State* L, const std::string& host, int port, std::string& err)
{
	Destroy();

	// 仅仅luajit需要
	mainStates.insert(L);

	emmyDebuggerManager->AddDebugger(L);

	const auto c = std::make_shared<SocketClientTransporter>();
	transporter = c;
	// c->SetHandler(shared_from_this());
	const auto suc = c->Connect(host, port, err);
	if (suc)
	{
		WaitIDE(true);
	}
	else
	{
		lua_pushcfunction(L, LuaError);
		lua_pushstring(L, err.c_str());
		lua_call(L, 1, 0);
	}
	return suc;
}

bool EmmyFacade::PipeListen(lua_State* L, const std::string& name, std::string& err)
{
	Destroy();

	// 仅仅luajit需要
	mainStates.insert(L);

	emmyDebuggerManager->AddDebugger(L);

	const auto p = std::make_shared<PipelineServerTransporter>();
	transporter = p;
	// p->SetHandler(shared_from_this());
	const auto suc = p->pipe(name, err);
	return suc;
}

bool EmmyFacade::PipeConnect(lua_State* L, const std::string& name, std::string& err)
{
	Destroy();

	// 仅仅luajit需要
	mainStates.insert(L);

	emmyDebuggerManager->AddDebugger(L);

	const auto p = std::make_shared<PipelineClientTransporter>();
	transporter = p;
	// p->SetHandler(shared_from_this());
	const auto suc = p->Connect(name, err);
	if (suc)
	{
		WaitIDE(true);
	}
	return suc;
}

void EmmyFacade::WaitIDE(bool force, int timeout)
{
	if (transporter != nullptr
		&& (transporter->IsServerMode() || force)
		&& !isWaitingForIDE
		&& !isIDEReady)
	{
		isWaitingForIDE = true;
		std::unique_lock<std::mutex> lock(waitIDEMutex);
		if (timeout > 0)
			waitIDECV.wait_for(lock, std::chrono::milliseconds(timeout));
		else
			waitIDECV.wait(lock);
		isWaitingForIDE = false;
	}
}

int EmmyFacade::BreakHere(lua_State* L)
{
	if (!isIDEReady)
		return 0;

	emmyDebuggerManager->HandleBreak(L);

	return 1;
}

int EmmyFacade::OnConnect(bool suc)
{
	return 0;
}

int EmmyFacade::OnDisconnect()
{
	isIDEReady = false;
	isWaitingForIDE = false;

	emmyDebuggerManager->OnDisconnect();

	if (workMode == WorkMode::Attach)
	{
		emmyDebuggerManager->RemoveAllDebugger();
	}

	return 0;
}

void EmmyFacade::Destroy()
{
	OnDisconnect();

	if (transporter)
	{
		transporter->Stop();
		transporter = nullptr;
	}
}

void EmmyFacade::SetWorkMode(WorkMode mode)
{
	workMode = mode;
}

WorkMode EmmyFacade::GetWorkMode()
{
	return workMode;
}

void EmmyFacade::OnReceiveMessage(const rapidjson::Document& document)
{
	const auto cmd = static_cast<MessageCMD>(document["cmd"].GetInt());
	switch (cmd)
	{
	case MessageCMD::InitReq:
		OnInitReq(document);
		break;
	case MessageCMD::ReadyReq:
		OnReadyReq(document);
		break;
	case MessageCMD::AddBreakPointReq:
		OnAddBreakPointReq(document);
		break;
	case MessageCMD::RemoveBreakPointReq:
		OnRemoveBreakPointReq(document);
		break;
	case MessageCMD::ActionReq:
		//assert(isIDEReady);
		OnActionReq(document);
		break;
	case MessageCMD::EvalReq:
		//assert(isIDEReady);
		OnEvalReq(document);
		break;
	default:
		break;
	}
}

void EmmyFacade::OnInitReq(const rapidjson::Document& document)
{
	if (StartHook)
	{
		StartHook();
	}

	if (document.HasMember("emmyHelper"))
	{
		emmyDebuggerManager->helperCode = document["emmyHelper"].GetString();
	}

	//file extension names: .lua, .txt, .lua.txt ...
	if (document.HasMember("ext"))
	{
		std::vector<std::string> extNames;
		const auto ext = document["ext"].GetArray();
		auto it = ext.begin();
		while (it != ext.end())
		{
			const auto extName = (*it).GetString();
			extNames.emplace_back(extName);
			++it;
		}

		emmyDebuggerManager->extNames = extNames;
	}

	emmyDebuggerManager->SetRunning(true);

	//TODO 这里有个线程安全问题，消息线程和lua 执行线程不是相同线程，但是没有一个锁能让我做同步
	// 所以我不能在这里访问lua state 指针的内部结构
	// 
	// 只能在消息线程设置hook
	// 但是存在一个问题只能给主lua_state 设置hook，如果此后进程一直在协程中执行则无法触发该hook
	// Attach函数内部设置了hook，存在一个线程问题，可能引发崩溃
	// 方案：提前为主state 设置hook 利用hook 实现同步
	auto debuggers = emmyDebuggerManager->GetDebuggers();
	for (auto debugger : debuggers)
	{
		debugger->Attach(false);
	}
}

void EmmyFacade::OnReadyReq(const rapidjson::Document& document)
{
	isIDEReady = true;
	waitIDECV.notify_all();
}

void FillVariables(rapidjson::Value& container, const std::vector<std::shared_ptr<Variable>>& variables,
                   rapidjson::MemoryPoolAllocator<>& allocator);

void FillVariable(rapidjson::Value& container, const std::shared_ptr<Variable> variable,
                  rapidjson::MemoryPoolAllocator<>& allocator)
{
	container.AddMember("name", variable->name, allocator);
	container.AddMember("nameType", variable->nameType, allocator);
	container.AddMember("value", variable->value, allocator);
	container.AddMember("valueType", variable->valueType, allocator);
	container.AddMember("valueTypeName", variable->valueTypeName, allocator);
	container.AddMember("cacheId", variable->cacheId, allocator);
	// children
	if (!variable->children.empty())
	{
		rapidjson::Value childrenValue(rapidjson::kArrayType);
		FillVariables(childrenValue, variable->children, allocator);
		container.AddMember("children", childrenValue, allocator);
	}
}

void FillVariables(rapidjson::Value& container, const std::vector<std::shared_ptr<Variable>>& variables,
                   rapidjson::MemoryPoolAllocator<>& allocator)
{
	std::vector<std::shared_ptr<Variable>> tmp = variables;
	for (auto variable : tmp)
	{
		rapidjson::Value variableValue(rapidjson::kObjectType);
		FillVariable(variableValue, variable, allocator);
		container.PushBack(variableValue, allocator);
	}
}

void FillStacks(rapidjson::Value& container, std::vector<std::shared_ptr<Stack>>& stacks,
                rapidjson::MemoryPoolAllocator<>& allocator)
{
	for (auto stack : stacks)
	{
		rapidjson::Value stackValue(rapidjson::kObjectType);
		stackValue.AddMember("file", stack->file, allocator);
		stackValue.AddMember("functionName", stack->functionName, allocator);
		stackValue.AddMember("line", stack->line, allocator);
		stackValue.AddMember("level", stack->level, allocator);

		// local variables
		rapidjson::Value localVariables(rapidjson::kArrayType);
		FillVariables(localVariables, stack->localVariables, allocator);
		stackValue.AddMember("localVariables", localVariables, allocator);
		// upvalue variables
		rapidjson::Value upvalueVariables(rapidjson::kArrayType);
		FillVariables(upvalueVariables, stack->upvalueVariables, allocator);
		stackValue.AddMember("upvalueVariables", upvalueVariables, allocator);

		container.PushBack(stackValue, allocator);
	}
}

bool EmmyFacade::OnBreak(std::shared_ptr<Debugger> debugger)
{
	if (!debugger)
	{
		return false;
	}
	std::vector<std::shared_ptr<Stack>> stacks;

	emmyDebuggerManager->SetBreakedDebugger(debugger);

	debugger->GetStacks(stacks, []()
	{
		return std::make_shared<Stack>();
	});

	rapidjson::Document document;
	document.SetObject();
	auto& allocator = document.GetAllocator();
	document.AddMember("cmd", static_cast<int>(MessageCMD::BreakNotify), allocator);
	//stacks
	rapidjson::Value stacksValue(rapidjson::kArrayType);
	FillStacks(stacksValue, stacks, allocator);
	document.AddMember("stacks", stacksValue, allocator);

	transporter->Send(int(MessageCMD::BreakNotify), document);

	return true;
}

void ReadBreakPoint(const rapidjson::Value& value, std::shared_ptr<BreakPoint> bp)
{
	if (value.HasMember("file"))
	{
		bp->file = value["file"].GetString();
	}
	if (value.HasMember("line"))
	{
		bp->line = value["line"].GetInt();
	}
	if (value.HasMember("condition"))
	{
		bp->condition = value["condition"].GetString();
	}
	if (value.HasMember("hitCondition"))
	{
		bp->hitCondition = value["hitCondition"].GetString();
	}
	if (value.HasMember("logMessage"))
	{
		bp->logMessage = value["logMessage"].GetString();
	}
}

void EmmyFacade::OnAddBreakPointReq(const rapidjson::Document& document)
{
	if (document.HasMember("clear"))
	{
		const auto all = document["clear"].GetBool();
		if (all)
		{
			emmyDebuggerManager->RemoveAllBreakPoints();
		}
	}
	if (document.HasMember("breakPoints"))
	{
		const auto docBreakPoints = document["breakPoints"].GetArray();
		auto it = docBreakPoints.begin();
		while (it != docBreakPoints.end())
		{
			auto bp = std::make_shared<BreakPoint>();
			ReadBreakPoint(*it, bp);

			bp->hitCount = 0;
			// ParsePathParts(bp->file, bp->pathParts);

			emmyDebuggerManager->AddBreakpoint(bp);

			++it;
		}
	}
	// todo: response
}

void EmmyFacade::OnRemoveBreakPointReq(const rapidjson::Document& document)
{
	if (document.HasMember("breakPoints"))
	{
		const auto breakPoints = document["breakPoints"].GetArray();
		auto it = breakPoints.begin();
		while (it != breakPoints.end())
		{
			auto bp = std::make_shared<BreakPoint>();
			ReadBreakPoint(*it, bp);
			emmyDebuggerManager->RemoveBreakpoint(bp->file, bp->line);
			++it;
		}
	}
	// todo: response
}

void EmmyFacade::OnActionReq(const rapidjson::Document& document)
{
	const auto action = static_cast<DebugAction>(document["action"].GetInt());

	emmyDebuggerManager->DoAction(action);
	// todo: response
}

void EmmyFacade::OnEvalReq(const rapidjson::Document& document)
{
	const auto seq = document["seq"].GetInt();
	const auto expr = document["expr"].GetString();
	const auto stackLevel = document["stackLevel"].GetInt();
	const auto depth = document["depth"].GetInt();
	auto cacheId = 0;
	if (document.HasMember("cacheId"))
	{
		cacheId = document["cacheId"].GetInt();
	}

	auto context = std::make_shared<EvalContext>();
	context->seq = seq;
	context->expr = expr;
	context->stackLevel = stackLevel;
	context->depth = depth;
	context->cacheId = cacheId;
	context->success = false;

	emmyDebuggerManager->Eval(context);
}

void EmmyFacade::OnEvalResult(std::shared_ptr<EvalContext> context)
{
	rapidjson::Document rspDoc;
	rspDoc.SetObject();
	auto& allocator = rspDoc.GetAllocator();
	rspDoc.AddMember("seq", context->seq, allocator);
	rspDoc.AddMember("success", context->success, allocator);
	if (context->success)
	{
		rapidjson::Value v(rapidjson::kObjectType);
		FillVariable(v, context->result, allocator);
		rspDoc.AddMember("value", v, allocator);
	}
	else
	{
		rspDoc.AddMember("error", context->error, allocator);
	}

	if (transporter)
	{
		transporter->Send(int(MessageCMD::EvalRsp), rspDoc);
	}
}

void EmmyFacade::SendLog(LogType type, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buff[1024] = {0};
	vsnprintf(buff, 1024, fmt, args);
	va_end(args);

	const std::string msg = buff;

	rapidjson::Document rspDoc;
	rspDoc.SetObject();
	auto& allocator = rspDoc.GetAllocator();
	rspDoc.AddMember("type", (int)type, allocator);
	rspDoc.AddMember("message", msg, allocator);
	if (transporter)
	{
		transporter->Send(int(MessageCMD::LogNotify), rspDoc);
	}
}

void EmmyFacade::OnLuaStateGC(lua_State* L)
{
	auto debugger = emmyDebuggerManager->RemoveDebugger(L);

	if (debugger)
	{
		debugger->Detach();
	}

	if (workMode == WorkMode::EmmyCore)
	{
		if (emmyDebuggerManager->IsDebuggerEmpty())
		{
			Destroy();
		}
	}
}

void EmmyFacade::Hook(lua_State* L, lua_Debug* ar)
{
	auto debugger = GetDebugger(L);
	if (debugger)
	{
		if (!debugger->IsRunning())
		{
			return;
		}

		debugger->Hook(ar, L);
	}
	else
	{
		if (workMode == WorkMode::Attach)
		{
			debugger = emmyDebuggerManager->AddDebugger(L);
			install_emmy_core(L);
			if (emmyDebuggerManager->IsRunning()) {
				debugger->Start();
				debugger->Attach();
			}
			// send attached notify
			rapidjson::Document rspDoc;
			rspDoc.SetObject();
			// fix macosx compiler error,
			// repidjson 应该有重载决议的错误
			int64_t state = reinterpret_cast<int64_t>(L);
			rspDoc.AddMember("state", state, rspDoc.GetAllocator());
			this->transporter->Send(int(MessageCMD::AttachedNotify), rspDoc);

			debugger->Hook(ar, L);
		}
	}
}

std::shared_ptr<EmmyDebuggerManager> EmmyFacade::GetDebugManager() const
{
	return emmyDebuggerManager;
}


std::shared_ptr<Variable> EmmyFacade::GetVariableRef(Variable* variable)
{
	auto it = luaVariableRef.find(variable);

	if (it != luaVariableRef.end())
	{
		return it->second;
	}
	else
	{
		return nullptr;
	}
}

void EmmyFacade::AddVariableRef(std::shared_ptr<Variable> variable)
{
	luaVariableRef.insert({variable.get(), variable});
}

void EmmyFacade::RemoveVariableRef(Variable* variable)
{
	auto it = luaVariableRef.find(variable);
	if (it != luaVariableRef.end())
	{
		luaVariableRef.erase(it);
	}
}

std::shared_ptr<Debugger> EmmyFacade::GetDebugger(lua_State* L)
{
	return emmyDebuggerManager->GetDebugger(L);
}

void EmmyFacade::StartupHookMode(int port)
{
	Destroy();

	// 1024 - 65535
	while (port > 0xffff) port -= 0xffff;
	while (port < 0x400) port += 0x400;

	const auto s = std::make_shared<SocketServerTransporter>();
	std::string err;
	const auto suc = s->Listen("localhost", port, err);
	if (suc)
	{
		transporter = s;
		// transporter->SetHandler(shared_from_this());
	}
}

void EmmyFacade::Attach(lua_State* L)
{
	if (!this->transporter->IsConnected())
		return;

	// 这里存在一个问题就是 hook 的时机太早了，globalstate 都还没初始化完毕

	if (!isAPIReady)
	{
		// 考虑到emmy_hook use lua source
		isAPIReady = install_emmy_core(L);
	}

	lua_sethook(L, EmmyFacade::HookLua, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);
	//
	// auto mainState = GetMainState(L);
	// auto debugger = emmyDebuggerManager->GetDebugger(L);
	//
	// if (!debugger)
	// {
	// 	debugger = emmyDebuggerManager->AddDebugger(L);
	//
	// 	debugger->Start();
	//
	// 	if (debugger->IsMainCoroutine(L) && !install)
	// 	{
	// 		install = install_emmy_core(L);
	// 	}
	//
	// 	debugger->Attach();
	//

	// }
	//
	// debugger->UpdateHook(LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, L);
}
