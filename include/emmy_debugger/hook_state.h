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

#include <string>
#include <memory>
#include "api/lua_api.h"

class Debugger;

class HookState {
protected:
	lua_State* currentStateL;
public:
	HookState();

	virtual ~HookState() {
	}

	virtual bool Start(std::shared_ptr<Debugger> debugger, lua_State* current);
	virtual void ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar);
};

// continue
class HookStateContinue : public HookState {
	bool Start(std::shared_ptr<Debugger> debugger, lua_State* current) override;
};

class StackLevelBasedState : public HookState {
protected:
	int oriStackLevel;
	int newStackLevel;
	bool Start(std::shared_ptr<Debugger> debugger, lua_State* current) override;
	void UpdateStackLevel(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar);
};

// step in
class HookStateStepIn : public StackLevelBasedState {
	std::string file;
	int line = 0;
	bool Start(std::shared_ptr<Debugger> debugger, lua_State* current) override;
	void ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar) override;
};

// step out
class HookStateStepOut : public StackLevelBasedState {
	bool Start(std::shared_ptr<Debugger> debugger, lua_State* current) override;
	void ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar) override;
};

// step over
class HookStateStepOver : public StackLevelBasedState {
	std::string file;
	int line = 0;

	bool Start(std::shared_ptr<Debugger> debugger, lua_State* current) override;
	void ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar) override;
};

// always break
class HookStateBreak : public HookState {
	void ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar) override;
};

// stop
class HookStateStop : public HookState {
	bool Start(std::shared_ptr<Debugger> debugger, lua_State* current) override;
};
