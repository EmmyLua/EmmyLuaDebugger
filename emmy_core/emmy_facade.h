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
#include "emmy_core.h"
#include <rapidjson/document.h>
#include <mutex>
#include <condition_variable>

class Transporter;
struct EvalContext;

class EmmyFacade {
	Transporter* transporter;
	lua_State* L;
	std::mutex waitIDEMutex;
	std::condition_variable waitIDECV;
public:
	static EmmyFacade* Get();
	EmmyFacade();
	~EmmyFacade();
	int TcpListen(lua_State* L, const std::string& host, int port);
	int TcpConnect(lua_State* L, const std::string& host, int port);
	int PipeListen(lua_State* L, const std::string& name);
	int PipeConnect(lua_State* L, const std::string& name);
	int BreakHere(lua_State* L);
	int OnConnect();
	int Stop();
	void WaitIDE();
	void OnReceiveMessage(const rapidjson::Document& document);
	void OnBreak();
	void Destroy();
	void OnEvalResult(const EvalContext* context);
private:
	void OnAddBreakPoint(const rapidjson::Document& document);
	void OnRemoveBreakPoint(const rapidjson::Document& document);
	void OnAction(const rapidjson::Document& document);
	void OnEval(const rapidjson::Document& document);
};
