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
#include "types.h"
#include <vector>
#include <set>
#include <mutex>
#include <condition_variable>
#include <queue>

typedef Stack* (*StackAllocatorCB)();
typedef void (*OnBreakCB)();
typedef void (*Executor)();

class HookState;

class EvalContext;

class Debugger {
	lua_State* L;
	lua_State* currentStateL;
	HookState* hookState;
	bool hooked;
	bool skipHook;
	bool blocking;
	std::vector<std::string> doStringList;
	std::vector<BreakPoint*> breakPoints;
	std::set<int> lineSet;
	std::mutex mutexRun;
	std::condition_variable cvRun;
	std::mutex mutexEval;
	std::queue<EvalContext*> evalQueue;
	std::mutex mutexBP;
	std::vector <std::string> extNames;

	HookState* stateBreak;
	HookState* stateStepOver;
	HookState* stateStepIn;
	HookState* stateStepOut;
	HookState* stateContinue;
	HookState* stateStop;

	friend class EmmyFacade;
	friend class StackLevelBasedState;
	friend class HookStateStepIn;
	friend class HookStateStepOut;
	friend class HookStateStepOver;
	friend class HookStateBreak;
	friend class HookStateStop;
public:
	static Debugger* Get();

	Debugger();
	~Debugger();

	void Start(lua_State* L);
	void Hook(lua_State* L, lua_Debug* ar);
	void Stop();
	void AddBreakPoint(const BreakPoint& breakPoint);
	void RemoveBreakPoint(const std::string& file, int line);
	void RemoveAllBreakpoints();
	void AsyncDoString(const char* code);
	bool Eval(EvalContext* evalContext, bool force = false);
	bool GetStacks(std::vector<Stack*>& stacks, StackAllocatorCB alloc);
	void GetVariable(Variable* variable, lua_State* L, int index, int depth, bool queryHelper = true);
	void DoAction(DebugAction action);
	void EnterDebugMode();
	void ExitDebugMode();
	void ExecuteWithSkipHook(Executor exec);
	void SetExtNames(const std::vector <std::string>& names);
private:
	BreakPoint* FindBreakPoint(lua_State* L, lua_Debug* ar);
	BreakPoint* FindBreakPoint(const std::string& file, int line);
	std::string GetFile(lua_Debug* ar) const;
	int GetStackLevel(lua_State* L, bool skipC) const;
	void RefreshLineSet();
	void UpdateHook(lua_State* L, int mask);
	void HandleBreak(lua_State* L);
	void SetHookState(HookState* newState);
	void CheckDoString();
	bool CreateEnv(int stackLevel);
	bool DoEval(EvalContext* evalContext);
	bool MatchFileName(const std::string& chunkName, const std::string& fileName) const;
	void CacheValue(lua_State* L, int valueIndex, Variable* variable) const;
	void ClearCache(lua_State* L) const;
};
