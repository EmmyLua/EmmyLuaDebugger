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

EmmyFacade* EmmyFacade::Get() {
	static EmmyFacade instance;
	return &instance;
}

EmmyFacade::EmmyFacade():
	transporter(nullptr),
	L(nullptr),
	isIDEReady(false) {
}

EmmyFacade::~EmmyFacade() {
	delete transporter;
}

int EmmyFacade::TcpListen(lua_State* L, const std::string& host, int port) {
	Destroy();
	this->L = L;
	const auto s = new SocketServerTransporter();
	transporter = s;
	s->SetHandler(this);
	const auto suc = s->Listen(host, port);
	return suc ? 0 : 1;
}

int EmmyFacade::TcpConnect(lua_State* L, const std::string& host, int port) {
	Destroy();
	this->L = L;
	const auto c = new SocketClientTransporter();
	transporter = c;
	c->SetHandler(this);
	const auto suc = c->Connect(host, port);
	return suc ? 0 : 1;
}

int EmmyFacade::PipeListen(lua_State* L, const std::string& name) {
	Destroy();
	this->L = L;
	const auto p = new PipelineServerTransporter();
	transporter = p;
	p->SetHandler(this);
	const auto suc = p->pipe(name);
	return suc ? 0 : 1;
}

int EmmyFacade::PipeConnect(lua_State* L, const std::string& name) {
	Destroy();
	this->L = L;
	const auto p = new PipelineClientTransporter();
	transporter = p;
	p->SetHandler(this);
	const auto suc = p->Connect(name);
	return suc ? 0 : 1;
}

void EmmyFacade::WaitIDE() {
	if (transporter != nullptr
		&& transporter->IsServerMode()
		&& !isIDEReady) {
		std::unique_lock<std::mutex> lock(waitIDEMutex);
		waitIDECV.wait(lock);
	}
}

int EmmyFacade::BreakHere(lua_State* L) {
	if (!isIDEReady)
		return 0;
	Debugger::Get()->HandleBreak(L);
	return 1;
}

int EmmyFacade::OnConnect() {
	return 0;
}

int EmmyFacade::OnDisconnect() {
	isIDEReady = false;
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
		assert(isIDEReady);
		OnActionReq(document);
		break;
	case MessageCMD::EvalReq:
		assert(isIDEReady);
		OnEvalReq(document);
		break;
	default:
		break;
	}
}

void EmmyFacade::OnInitReq(const rapidjson::Document& document) {
	Debugger::Get()->Start(L);
	if (document.HasMember("emmyHelper")) {
		const auto code = document["emmyHelper"].GetString();
		auto context = new EvalContext();
		context->seq = -1;
		context->expr = code;
		context->stackLevel = 0;
		context->depth = 0;
		context->success = false;

		Debugger::Get()->Eval(context, true);
	}
	isIDEReady = true;
	waitIDECV.notify_all();
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

void EmmyFacade::OnBreak() {
	std::vector<Stack*> stacks;
	Debugger::Get()->GetStacks(stacks, []() {
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

	auto context = new EvalContext();
	context->seq = seq;
	context->expr = expr;
	context->stackLevel = stackLevel;
	context->depth = depth;
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
