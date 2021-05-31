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

#include <vector>
#include <set>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <memory>
#include "api/lua_api.h"
#include "emmy_debugger/hook_state.h"
#include "types.h"

using StackAllocatorCB = std::shared_ptr<Stack> (*)();
using OnBreakCB = void (*)();
using Executor = std::function<void(lua_State* L)> ;

class Debugger : public std::enable_shared_from_this<Debugger>
{
	lua_State* L;
	std::shared_ptr<HookState> hookState;

	bool running;
	bool skipHook;
	bool blocking;


	std::vector<std::string> doStringList;
	std::vector<std::shared_ptr<BreakPoint>> breakPoints;
	std::set<int> lineSet;
	std::mutex mutexRun;
	std::condition_variable cvRun;
	std::mutex mutexEval;
	std::queue<std::shared_ptr<EvalContext>> evalQueue;
	std::mutex mutexBP;

	std::mutex mutexLuaThread;
	std::vector<Executor> luaThreadExecutors;



	friend class EmmyFacade;
	friend class StackLevelBasedState;
	friend class HookStateStepIn;
	friend class HookStateStepOut;
	friend class HookStateStepOver;
	friend class HookStateBreak;
	friend class HookStateStop;
public:
	Debugger(lua_State* L);
	~Debugger();

	void Start();
	void Attach(bool doHelperCode=true);
	void Detach();
	void Hook(lua_Debug* ar);
	void Stop();
	bool IsRunning() const;
	void AddBreakPoint(std::shared_ptr<BreakPoint> breakPoint);
	void RemoveBreakPoint(const std::string& file, int line);
	void RemoveAllBreakpoints();
	void AsyncDoString(const std::string& code);
	bool Eval(std::shared_ptr<EvalContext> evalContext, bool force = false);
	bool GetStacks(std::vector<std::shared_ptr<Stack>>& stacks, StackAllocatorCB alloc);
	void GetVariable(std::shared_ptr<Variable> variable, int index, int depth, bool queryHelper = true);
	void DoAction(DebugAction action);
	void EnterDebugMode();
	void ExitDebugMode();
	void ExecuteWithSkipHook(const Executor& exec);
	void ExecuteOnLuaThread(const Executor& exec);

private:
	std::shared_ptr<BreakPoint> FindBreakPoint(lua_Debug* ar);
	std::shared_ptr<BreakPoint> FindBreakPoint(const std::string& file, int line);
	std::string GetFile(lua_Debug* ar) const;
	int GetStackLevel(bool skipC) const;
	void RefreshLineSet();
	void UpdateHook(int mask);
	void HandleBreak();
	void SetHookState(std::shared_ptr<HookState> newState);
	void CheckDoString();
	bool CreateEnv(lua_State* L, int stackLevel);
	bool ProcessBreakPoint(std::shared_ptr<BreakPoint> bp);
	bool DoEval(std::shared_ptr<EvalContext> evalContext);
	bool MatchFileName(const std::string& chunkName, const std::string& fileName) const;
	void CacheValue(int valueIndex, std::shared_ptr<Variable> variable) const;
	void ClearCache() const;
};
