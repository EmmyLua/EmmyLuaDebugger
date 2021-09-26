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
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <memory>
#include <set>

#include "api/lua_api.h"
#include "emmy_debugger/hook_state.h"
#include "types.h"

using StackAllocatorCB = std::shared_ptr<Stack> (*)();
using Executor = std::function<void(lua_State* L)>;
class EmmyDebuggerManager;

class Debugger : public std::enable_shared_from_this<Debugger>
{
public:
	Debugger(lua_State* L, std::shared_ptr<EmmyDebuggerManager> manager);
	~Debugger();

	void Start();
	/*
	 * @param isMainThread 代表是否是在主线程执行
	 */
	void Attach(bool isMainThread = true);
	/*
	 * 主lua state 销毁时，所做工作
	 */
	void Detach();

	void SetCurrentState(lua_State* L);
	/*
	 * hook时调用
	 */
	void Hook(lua_Debug* ar, lua_State* L);
	/*
	 * 停止调试时调用
	 */
	void Stop();
	/*
	 * 调试器是否运行
	 */
	bool IsRunning() const;
	/*
	 * 判断当前使用的lua_state 是否是main state
	 */
	bool IsMainCoroutine(lua_State* L) const;
	/*
	 * 推迟到lua线程执行
	 */
	void AsyncDoString(const std::string& code);
	bool Eval(std::shared_ptr<EvalContext> evalContext, bool force = false);
	bool GetStacks(std::vector<std::shared_ptr<Stack>>& stacks, StackAllocatorCB alloc);
	void GetVariable(std::shared_ptr<Variable> variable, int index, int depth, bool queryHelper = true);
	void DoAction(DebugAction action);
	void EnterDebugMode();
	void ExitDebugMode();
	void ExecuteWithSkipHook(const Executor& exec);
	void ExecuteOnLuaThread(const Executor& exec);
	void HandleBreak();
	int GetStackLevel(bool skipC) const;
	/*
	 * 更新hook
	 */
	void UpdateHook(int mask, lua_State* L);
	/*
	 * 设置初始化hook
	 */
	void SetInitHook();

	/*
	 * 该思路暂时未启用
	 * 设置等待连接的hook
	 */
	void SetWaitConnectedHook(lua_State* L);

	/*
	 * 设置当前状态机，他的锁由doAction负责
	 */
	void SetHookState(std::shared_ptr<HookState> newState);
	std::shared_ptr<EmmyDebuggerManager> GetEmmyDebuggerManager();

private:
	std::shared_ptr<BreakPoint> FindBreakPoint(lua_Debug* ar);
	std::shared_ptr<BreakPoint> FindBreakPoint(const std::string& file, int line);
	std::string GetFile(lua_Debug* ar) const;

	void CheckDoString();
	bool CreateEnv(int stackLevel);
	bool ProcessBreakPoint(std::shared_ptr<BreakPoint> bp);
	bool DoEval(std::shared_ptr<EvalContext> evalContext);
	void DoLogMessage(std::shared_ptr<BreakPoint> bp);
	bool DoHitCondition(std::shared_ptr<BreakPoint> bp);
	// 模糊匹配算法会算出匹配度
	// 当多个文件路径都有可能命中应该采用匹配度最高的路径
	int FuzzyMatchFileName(const std::string& chunkName, const std::string& fileName) const;
	void CacheValue(int valueIndex, std::shared_ptr<Variable> variable) const;
	// bool HasCacheValue(int valueIndex) const;
	void ClearCache() const;

	// std::mutex waitConnectedMtx;
	
	lua_State* currentL;
	lua_State* mainL;

	std::shared_ptr<EmmyDebuggerManager> manager;

	// 取消递归锁的使用
	std::mutex hookStateMtx;
	std::shared_ptr<HookState> hookState;

	bool running;
	bool skipHook;
	bool blocking;

	std::vector<std::string> doStringList;

	std::mutex runMtx;
	std::condition_variable cvRun;

	std::mutex luaThreadMtx;
	std::vector<Executor> luaThreadExecutors;

	std::mutex evalMtx;
	std::queue<std::shared_ptr<EvalContext>> evalQueue;


};
