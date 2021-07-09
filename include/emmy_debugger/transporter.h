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

#include <thread>
#include "uv.h"
#include "rapidjson/document.h"
class EmmyFacade;

enum class MessageCMD {
	Unknown,

	InitReq,
	InitRsp,

	ReadyReq,
	ReadyRsq,

	AddBreakPointReq,
	AddBreakPointRsp,

	RemoveBreakPointReq,
	RemoveBreakPointRsp,

	ActionReq,
	ActionRsp,

	EvalReq,
	EvalRsp,

	// debugger -> ide
	BreakNotify,
	AttachedNotify,

	StartHookReq,
	StartHookRsp,

	// debugger -> ide
	LogNotify,
};

class Transporter {
	std::thread thread;
	char* buf;
	size_t bufSize;
	size_t receiveSize;
	bool readHead;
	bool running;
	bool connected;
	bool serverMode;
protected:
	uv_loop_t* loop;
public:
	Transporter(bool server);
	virtual ~Transporter();
	virtual int Stop();
	bool IsConnected() const;
	bool IsServerMode() const;
	void Send(int cmd, const rapidjson::Document& document);
	// void SetHandler(std::shared_ptr<EmmyFacade> facade);
	void OnAfterRead(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf);
protected:
	virtual void Send(int cmd, const char* data, size_t len) = 0;
	void Send(uv_stream_t* handler, int cmd, const char* data, size_t len);
	// send raw data
	void Send(uv_stream_t* handler, const char* data, size_t len);
	void Receive(const char* data, size_t len);
	void OnReceiveMessage(const rapidjson::Document& document);
	void StartEventLoop();
	void Run();
	virtual void OnDisconnect();
	virtual void OnConnect(bool suc);
};
