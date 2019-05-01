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
#include <string>

class Debugger;

class HookState {
protected:
	lua_State * currentStateL;
public:
	HookState();
	virtual ~HookState() {}
	virtual void Start(Debugger* debugger, lua_State* L, lua_State* current);
	virtual void ProcessHook(Debugger* debugger, lua_State* L, lua_Debug* ar);
};

// continue
class HookStateContinue : public HookState {
	void Start(Debugger* debugger, lua_State* L, lua_State* current) override;
};

class StackLevelBasedState : public HookState {
protected:
	int oriStackLevel;
	int newStackLevel;
	int oldStackLevel;
	bool isRetPrevTime;
	bool isTailCall;
	void Start(Debugger* debugger, lua_State* L, lua_State* current) override;
	void UpdateStackLevel(Debugger* debugger, lua_State* L, lua_Debug* ar);
};

// step in
class HookStateStepIn : public StackLevelBasedState {
	void Start(Debugger* debugger, lua_State* L, lua_State* current) override;
	void ProcessHook(Debugger* debugger, lua_State* L, lua_Debug* ar) override;
};

// step out
class HookStateStepOut : public StackLevelBasedState {
	void Start(Debugger* debugger, lua_State* L, lua_State* current) override;
	void ProcessHook(Debugger* debugger, lua_State* L, lua_Debug* ar) override;
};

// step over
class HookStateStepOver : public StackLevelBasedState {
	std::string file;
	int line = 0;

	void Start(Debugger* debugger, lua_State* L, lua_State* current) override;
	void ProcessHook(Debugger* debugger, lua_State* L, lua_Debug* ar) override;
};

// always break
class HookStateBreak : public HookState {
	void ProcessHook(Debugger* debugger, lua_State* L, lua_Debug* ar) override;
};

// stop
class HookStateStop : public HookState {
	void Start(Debugger* debugger, lua_State* L, lua_State* current) override;
};