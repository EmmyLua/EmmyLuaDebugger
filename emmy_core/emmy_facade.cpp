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
#include "emmy_facade.h"
#include "emmy_debugger.h"
#include "transporter.h"
#include "proto/socket_server_transporter.h"
#include "proto/socket_client_transporter.h"
#include "proto/pipeline_server_transporter.h"
#include "proto/pipeline_client_transporter.h"
#include <cstdarg>

EmmyFacade* EmmyFacade::Get() {
	static EmmyFacade instance;
	return &instance;
}

EmmyFacade::EmmyFacade():
	transporter(nullptr),
	isIDEReady(false),
	isAPIReady(false),
	isWaitingForIDE(false) {
}

EmmyFacade::~EmmyFacade() {
	delete transporter;
}

extern "C" bool SetupLuaAPI();
bool EmmyFacade::SetupLuaAPI() {
	isAPIReady = ::SetupLuaAPI();
	return isAPIReady;
}

int LuaError(lua_State* L) {
	std::string msg = lua_tostring(L, 1);
	msg = "[Emmy]" + msg;
	lua_getglobal(L, "error");
	lua_pushstring(L, msg.c_str());
	lua_call(L, 1, 0);
	return 0;
}

int LuaPrint(lua_State* L) {
	std::string msg = lua_tostring(L, 1);
	msg = "[Emmy]" + msg;
	lua_getglobal(L, "print");
	lua_pushstring(L, msg.c_str());
	lua_call(L, 1, 0);
	return 0;
}

bool EmmyFacade::TcpListen(lua_State* L, const std::string& host, int port, std::string& err) {
	Destroy();
	states.insert(L);
	const auto s = new SocketServerTransporter();
	transporter = s;
	s->SetHandler(this);
	const auto suc = s->Listen(host, port, err);
	if (!suc) {
		lua_pushcfunction(L, LuaError);
		lua_pushstring(L, err.c_str());
		lua_call(L, 1, 0);
	}
	return suc;
}

bool EmmyFacade::TcpConnect(lua_State* L, const std::string& host, int port, std::string& err) {
	Destroy();
	states.insert(L);
	const auto c = new SocketClientTransporter();
	transporter = c;
	c->SetHandler(this);
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

bool EmmyFacade::PipeListen(lua_State* L, const std::string& name, std::string& err) {
	Destroy();
	states.insert(L);
	const auto p = new PipelineServerTransporter();
	transporter = p;
	p->SetHandler(this);
	const auto suc = p->pipe(name, err);
	return suc;
}

bool EmmyFacade::PipeConnect(lua_State* L, const std::string& name, std::string& err) {
	Destroy();
	states.insert(L);
	const auto p = new PipelineClientTransporter();
	transporter = p;
	p->SetHandler(this);
	const auto suc = p->Connect(name, err);
	if (suc) {
		WaitIDE(true);
	}
	return suc;
}

void EmmyFacade::WaitIDE(bool force) {
	if (transporter != nullptr
		&& (transporter->IsServerMode() || force)
		&& !isWaitingForIDE
		&& !isIDEReady) {
		isWaitingForIDE = true;
		std::unique_lock<std::mutex> lock(waitIDEMutex);
		waitIDECV.wait(lock);
		isWaitingForIDE = false;
	}
}

int EmmyFacade::BreakHere(lua_State* L) {
	if (!isIDEReady)
		return 0;
	Debugger::Get()->HandleBreak(L);
	return 1;
}

int EmmyFacade::OnConnect(bool suc) {
	return 0;
}

int EmmyFacade::OnDisconnect() {
	isIDEReady = false;
	isWaitingForIDE = false;
	states.clear();
	Debugger::Get()->Stop();
	return 0;
}

void EmmyFacade::Destroy() {
	OnDisconnect();
	if (transporter) {
		transporter->Stop();
		delete transporter;
		transporter = nullptr;
	}
}

void EmmyFacade::OnReceiveMessage(const rapidjson::Document& document) {
	const auto cmd = static_cast<MessageCMD>(document["cmd"].GetInt());
	switch (cmd) {
#if EMMY_BUILD_AS_HOOK
	case MessageCMD::StartHookReq:
		StartHook();
		break;
#endif
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

void EmmyFacade::OnInitReq(const rapidjson::Document& document) {
	if (document.HasMember("emmyHelper")) {
		helperCode = document["emmyHelper"].GetString();
		//Debugger::Get()->AsyncDoString(helperCode.c_str());
	}
	Debugger::Get()->Start(helperCode);
	for (auto L : states)
		Debugger::Get()->Attach(L);
	if (document.HasMember("ext")) {
		std::vector<std::string> extNames;
		const auto ext = document["ext"].GetArray();
		auto it = ext.begin();
		while (it != ext.end()) {
			const auto extName = (*it).GetString();
			extNames.emplace_back(extName);
			++it;
		}
		Debugger::Get()->SetExtNames(extNames);
	}
}

void EmmyFacade::OnReadyReq(const rapidjson::Document& document) {
	isIDEReady = true;
	waitIDECV.notify_all();
}

void FillVariables(rapidjson::Value& container, const std::vector<Variable*>& variables,
                   rapidjson::MemoryPoolAllocator<>& allocator);

void FillVariable(rapidjson::Value& container, const Variable* variable, rapidjson::MemoryPoolAllocator<>& allocator) {
	container.AddMember("name", variable->name, allocator);
	container.AddMember("nameType", variable->nameType, allocator);
	container.AddMember("value", variable->value, allocator);
	container.AddMember("valueType", variable->valueType, allocator);
	container.AddMember("valueTypeName", variable->valueTypeName, allocator);
	container.AddMember("cacheId", variable->cacheId, allocator);
	// children
	if (!variable->children.empty()) {
		rapidjson::Value childrenValue(rapidjson::kArrayType);
		FillVariables(childrenValue, variable->children, allocator);
		container.AddMember("children", childrenValue, allocator);
	}
}

void FillVariables(rapidjson::Value& container, const std::vector<Variable*>& variables,
                   rapidjson::MemoryPoolAllocator<>& allocator) {
	std::vector<Variable*> tmp = variables;
	for (auto variable : tmp) {
		rapidjson::Value variableValue(rapidjson::kObjectType);
		FillVariable(variableValue, variable, allocator);
		container.PushBack(variableValue, allocator);
	}
}

void FillStacks(rapidjson::Value& container, std::vector<Stack*>& stacks, rapidjson::MemoryPoolAllocator<>& allocator) {
	for (auto stack : stacks) {
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

void EmmyFacade::OnBreak(lua_State* L) {
	std::vector<Stack*> stacks;
	Debugger::Get()->GetStacks(L, stacks, []() {
		return new Stack();
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

	for (auto stack : stacks) {
		delete stack;
	}
}

void ReadBreakPoint(const rapidjson::Value& value, BreakPoint* bp) {
	if (value.HasMember("file")) {
		bp->file = value["file"].GetString();
	}
	if (value.HasMember("line")) {
		bp->line = value["line"].GetInt();
	}
}

void EmmyFacade::OnAddBreakPointReq(const rapidjson::Document& document) {
	if (document.HasMember("clear")) {
		const auto all = document["clear"].GetBool();
		if (all) {
			Debugger::Get()->RemoveAllBreakpoints();
		}
	}
	if (document.HasMember("breakPoints")) {
		const auto breakPoints = document["breakPoints"].GetArray();
		auto it = breakPoints.begin();
		while (it != breakPoints.end()) {
			BreakPoint bp;
			ReadBreakPoint(*it, &bp);
			Debugger::Get()->AddBreakPoint(bp);
			++it;
		}
	}
	// todo: response
}

void EmmyFacade::OnRemoveBreakPointReq(const rapidjson::Document& document) {
	if (document.HasMember("breakPoints")) {
		const auto breakPoints = document["breakPoints"].GetArray();
		auto it = breakPoints.begin();
		while (it != breakPoints.end()) {
			BreakPoint bp;
			ReadBreakPoint(*it, &bp);
			Debugger::Get()->RemoveBreakPoint(bp.file, bp.line);
			++it;
		}
	}
	// todo: response
}

void EmmyFacade::OnActionReq(const rapidjson::Document& document) {
	const auto action = static_cast<DebugAction>(document["action"].GetInt());
	Debugger::Get()->DoAction(action);
	// todo: response
}

void EmmyFacade::OnEvalReq(const rapidjson::Document& document) {
	const auto seq = document["seq"].GetInt();
	const auto expr = document["expr"].GetString();
	const auto stackLevel = document["stackLevel"].GetInt();
	const auto depth = document["depth"].GetInt();
	auto cacheId = 0;
	if (document.HasMember("cacheId")) {
		cacheId = document["cacheId"].GetInt();
	}

	auto context = new EvalContext();
	context->seq = seq;
	context->expr = expr;
	context->stackLevel = stackLevel;
	context->depth = depth;
	context->cacheId = cacheId;
	context->success = false;
	Debugger::Get()->Eval(context);
}

void EmmyFacade::OnEvalResult(EvalContext* context) {
	rapidjson::Document rspDoc;
	rspDoc.SetObject();
	auto& allocator = rspDoc.GetAllocator();
	rspDoc.AddMember("seq", context->seq, allocator);
	rspDoc.AddMember("success", context->success, allocator);
	if (context->success) {
		rapidjson::Value v(rapidjson::kObjectType);
		FillVariable(v, &context->result, allocator);
		rspDoc.AddMember("value", v, allocator);
	}
	else {
		rspDoc.AddMember("error", context->error, allocator);
	}
	if (transporter)
		transporter->Send(int(MessageCMD::EvalRsp), rspDoc);
	delete context;
}

void EmmyFacade::SendLog(LogType type, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	char buff[1024] = { 0 };
	vsnprintf(buff, 1024, fmt, args);
	va_end(args);

	const std::string msg = buff;

	rapidjson::Document rspDoc;
	rspDoc.SetObject();
	auto& allocator = rspDoc.GetAllocator();
	rspDoc.AddMember("type", (int)type, allocator);
	rspDoc.AddMember("message", msg, allocator);
	if (transporter)
		transporter->Send(int(MessageCMD::LogNotify), rspDoc);
}

void EmmyFacade::OnLuaStateGC(lua_State* L) {
	const auto it = states.find(L);
	if (it != states.end())
		states.erase(it);
	Debugger::Get()->Detach(L);
	if (!states.empty())
		return;
#if EMMY_BUILD_AS_HOOK
	OnDisconnect();
#else
	Destroy();
#endif
}
