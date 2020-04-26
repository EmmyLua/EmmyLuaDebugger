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
#ifdef EMMY_BUILD_AS_HOOK
#include "emmy_facade.h"
#include "emmy_debugger.h"
#include "proto/socket_server_transporter.h"
#include "hook/emmy_hook.h"
#include <set>

extern "C" {
	bool install_emmy_core(struct lua_State* L);
}

void EmmyFacade::StartupHookMode(int port) {
	Destroy();

	// 1024 - 65535
	while (port > 0xffff) port -= 0xffff;
	while (port < 0x400) port += 0x400;

	const auto s = new SocketServerTransporter();
	std::string err;
	const auto suc = s->Listen("localhost", port, err);
	if (suc) {
		transporter = s;
		transporter->SetHandler(this);
	}
}

void EmmyFacade::Attach(lua_State* L) {
	if (!this->transporter->IsConnected())
		return;
	if (this->states.find(L) == this->states.end()) {
		this->states.insert(L);
		install_emmy_core(L);
		if (Debugger::Get()->IsRunning()) {
			Debugger::Get()->Attach(L);
		}

		// send attached notify
		rapidjson::Document rspDoc;
		rspDoc.SetObject();
		rspDoc.AddMember("state", (size_t)L, rspDoc.GetAllocator());
		this->transporter->Send(int(MessageCMD::AttachedNotify), rspDoc);
	}
}

void EmmyFacade::AttachThread(lua_State* L, int mask) {
	Debugger::Get()->UpdateHook(L, mask);
}

void EmmyFacade::StartHook() {
	FindAndHook();
}

#endif