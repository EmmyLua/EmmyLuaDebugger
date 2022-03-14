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
#include "emmy_debugger/emmy_debugger.h"
#include <algorithm>
#include <sstream>
#include "emmy_debugger/emmy_facade.h"
#include "emmy_debugger/hook_state.h"
#include "emmy_debugger/emmy_helper.h"
#include "emmy_debugger/lua_version.h"

#define CACHE_TABLE_NAME "_emmy_cache_table_"
#define CACHE_QUERY_NAME "_emmy_query_table_"

int cacheId = 1;

void WaitConnectedHook(lua_State* L, lua_Debug* ar)
{
	// EmmyFacade::Get()
	// std::lock_guard<std::mutex> lock()
}

Debugger::Debugger(lua_State* L, std::shared_ptr<EmmyDebuggerManager> manager):
	currentL(L),
	mainL(L),
	manager(manager),
	hookState(nullptr),
	running(false),
	skipHook(false),
	blocking(false)
{
}

Debugger::~Debugger()
{
}

void Debugger::Start()
{
	skipHook = false;
	blocking = false;
	running = true;
	doStringList.clear();
}

void Debugger::Attach()
{
	if (!running)
		return;

	// execute helper code
	if (!manager->helperCode.empty())
	{
		ExecuteOnLuaThread([this](lua_State* L)
		{
			const int t = lua_gettop(L);
			// 判断是不是主lua_state
			int ret = lua_pushthread(L);
			if (ret == 1)
			{
				const int r = luaL_loadstring(L, manager->helperCode.c_str());
				if (r == LUA_OK)
				{
					if (lua_pcall(L, 0, 0, 0) != LUA_OK)
					{
						std::string msg = lua_tostring(L, -1);
						printf("msg: %s", msg.c_str());
					}
				}
			}
			lua_settop(L, t);
		});
	}
}

void Debugger::Detach()
{
	// states.clear();
}


void Debugger::SetCurrentState(lua_State* L)
{
	currentL = L;
}

void Debugger::Hook(lua_Debug* ar, lua_State* L)
{
	if (skipHook)
	{
		return;
	}
	// 设置当前协程
	currentL = L;

	if (getDebugEvent(ar) == LUA_HOOKLINE)
	{
		// 对luaTreadExecutors 执行加锁
		{
			std::unique_lock<std::mutex> lock(luaThreadMtx);
			if (!luaThreadExecutors.empty())
			{
				for (auto& executor : luaThreadExecutors)
				{
					ExecuteWithSkipHook(executor);
				}
				luaThreadExecutors.clear();
			}
		}
		auto bp = FindBreakPoint(ar);
		if (bp && ProcessBreakPoint(bp))
		{
			HandleBreak();
			return;
		}
		// 加锁

		std::shared_ptr<HookState> state = nullptr;

		{
			std::lock_guard<std::mutex> lock(hookStateMtx);
			state = hookState;
		}

		if (state)
		{
			state->ProcessHook(shared_from_this(), currentL, ar);
		}
	}
}


void Debugger::Stop()
{
	running = false;
	skipHook = true;
	blocking = false;

	// 停止main_state 的hook
	// 但不停止coroutine的hook因为没有办法知道这个lua state 指针是否有效
	// stop在交互线程，不应该设置hook
	// 而且清理对lua_state 的 hook没有必要
	// UpdateHook(0, mainL);

	// 清理hook 状态
	{
		std::lock_guard<std::mutex> lock(hookStateMtx);
		hookState = nullptr;
	}

	{
		// clear lua thread executors
		std::unique_lock<std::mutex> luaThreadLock(luaThreadMtx);
		luaThreadExecutors.clear();
	}
	ExitDebugMode();
}

bool Debugger::IsRunning() const
{
	return running;
}

bool Debugger::IsMainCoroutine(lua_State* L) const
{
	return L == mainL;
}

bool Debugger::GetStacks(std::vector<std::shared_ptr<Stack>>& stacks, StackAllocatorCB alloc)
{
	if (!currentL)
	{
		return false;
	}

	auto L = currentL;

	int level = 0;
	while (true)
	{
		lua_Debug ar{};
		if (!lua_getstack(L, level, &ar))
		{
			break;
		}
		if (!lua_getinfo(L, "nSlu", &ar))
		{
			continue;
		}
		auto stack = alloc();
		stack->file = GetFile(&ar);
		stack->functionName = getDebugName(&ar) == nullptr ? "" : getDebugName(&ar);
		stack->level = level;
		stack->line = getDebugCurrentLine(&ar);
		stacks.push_back(stack);
		// get variables
		{
			for (int i = 1;; i++)
			{
				const char* name = lua_getlocal(L, &ar, i);
				if (name == nullptr)
				{
					break;
				}
				if (name[0] == '(')
				{
					lua_pop(L, 1);
					continue;
				}

				// add local variable
				auto var = stack->CreateVariable();
				var->name = name;
				GetVariable(var, -1, 1);
				lua_pop(L, 1);
				stack->localVariables.push_back(var);
			}

			if (lua_getinfo(L, "f", &ar))
			{
				const int fIdx = lua_gettop(L);
				for (int i = 1;; i++)
				{
					const char* name = lua_getupvalue(L, fIdx, i);
					if (!name)
					{
						break;
					}

					// add up variable
					auto var = stack->CreateVariable();
					var->name = name;
					GetVariable(var, -1, 1);
					lua_pop(L, 1);
					stack->upvalueVariables.push_back(var);
				}
				// pop function
				lua_pop(L, 1);
			}
		}

		level++;
	}
	return false;
}

bool CallMetaFunction(lua_State* L, int valueIndex, const char* method, int numResults, int& result)
{
	if (lua_getmetatable(L, valueIndex))
	{
		const int metaIndex = lua_gettop(L);
		if (!lua_isnil(L, metaIndex))
		{
			lua_pushstring(L, method);
			lua_rawget(L, metaIndex);
			if (lua_isnil(L, -1))
			{
				// The meta-method doesn't exist.
				lua_pop(L, 1);
				lua_remove(L, metaIndex);
				return false;
			}
			lua_pushvalue(L, valueIndex);
			result = lua_pcall(L, 1, numResults, 0);
		}
		lua_remove(L, metaIndex);
		return true;
	}
	return false;
}

std::string ToPointer(lua_State* L, int index)
{
	const void* pointer = lua_topointer(L, index);
	std::stringstream ss;
	ss << lua_typename(L, lua_type(L, index)) << "(0x" << std::hex << pointer << ")";
	return ss.str();
}

// algorithm optimization
void Debugger::GetVariable(std::shared_ptr<Variable> variable, int index, int depth, bool queryHelper)
{
	if (!currentL)
	{
		return;
	}

	auto L = currentL;

	// 如果没有计算深度则不予计算
	if (depth <= 0)
	{
		return;
	}

	const int topIndex = lua_gettop(L);
	index = lua_absindex(L, index);
	CacheValue(index, variable);
	const int type = lua_type(L, index);
	const char* typeName = lua_typename(L, type);
	variable->valueTypeName = typeName;
	variable->valueType = type;


	if (queryHelper && (type == LUA_TTABLE || type == LUA_TUSERDATA))
	{
		if (query_variable(L, variable, typeName, index, depth))
		{
			return;
		}
	}
	switch (type)
	{
	case LUA_TNIL:
		{
			variable->value = "nil";
			break;
		}
	case LUA_TNUMBER:
		{
			variable->value = lua_tostring(L, index);
			break;
		}
	case LUA_TBOOLEAN:
		{
			const bool v = lua_toboolean(L, index);
			variable->value = v ? "true" : "false";
			break;
		}
	case LUA_TSTRING:
		{
			variable->value = lua_tostring(L, index);
			break;
		}
	case LUA_TUSERDATA:
		{
			auto* string = lua_tostring(L, index);
			if (string == nullptr)
			{
				int result;
				if (CallMetaFunction(L, topIndex, "__tostring", 1, result) && result == 0)
				{
					string = lua_tostring(L, -1);
					lua_pop(L, 1);
				}
			}
			if (string)
			{
				variable->value = string;
			}
			else
			{
				variable->value = ToPointer(L, index);
			}
			if (depth > 1)
			{
				if (lua_getmetatable(L, index))
				{
					GetVariable(variable, -1, depth);
					lua_pop(L, 1); //pop meta
				}
			}
			break;
		}
	case LUA_TFUNCTION:
	case LUA_TLIGHTUSERDATA:
	case LUA_TTHREAD:
		{
			variable->value = ToPointer(L, index);
			break;
		}
	case LUA_TTABLE:
		{
			std::size_t tableSize = 0;
			const void* tableAddr = lua_topointer(L, index);
			lua_pushnil(L);
			while (lua_next(L, index))
			{
				// k: -2, v: -1
				if (depth > 1)
				{
					//todo: use allocator
					auto v = std::make_shared<Variable>();
					const auto t = lua_type(L, -2);
					v->nameType = t;
					if (t == LUA_TSTRING || t == LUA_TNUMBER || t == LUA_TBOOLEAN)
					{
						lua_pushvalue(L, -2); // avoid error: "invalid key to 'next'" ???
						v->name = lua_tostring(L, -1);
						lua_pop(L, 1);
					}
					else
					{
						v->name = ToPointer(L, -2);
					}
					GetVariable(v, -1, depth - 1);
					variable->children.push_back(v);
				}
				lua_pop(L, 1);
				tableSize++;
			}


			if (lua_getmetatable(L, index))
			{
				// metatable
				auto metatable = std::make_shared<Variable>();
				metatable->name = "(metatable)";
				metatable->nameType = LUA_TSTRING;

				GetVariable(metatable, -1, depth - 1);
				variable->children.push_back(metatable);

				//__index
				{
					lua_getfield(L, -1, "__index");
					if (!lua_isnil(L, -1))
					{
						auto v = std::make_shared<Variable>();
						v->name = "(metatable.__index)";
						v->nameType = LUA_TSTRING;
						GetVariable(v, -1, depth - 1);
						variable->children.push_back(v);
						// if (depth > 1)
						// {
						// 	for (auto child : v->children)
						// 	{
						// 		variable->children.push_back(child->Clone());
						// 	}
						// }
						// tableSize += v->children.size();
					}
					lua_pop(L, 1);
				}

				// metatable
				lua_pop(L, 1);
			}

			std::stringstream ss;
			ss << "table(0x" << std::hex << tableAddr << std::dec << ", size = " << tableSize << ")";
			variable->value = ss.str();
			break;
		}
	}
	const int t2 = lua_gettop(L);
	assert(t2 == topIndex);
}

void Debugger::CacheValue(int valueIndex, std::shared_ptr<Variable> variable) const
{
	if (!currentL)
	{
		return;
	}

	auto L = currentL;

	const int type = lua_type(L, valueIndex);
	if (type == LUA_TUSERDATA || type == LUA_TTABLE)
	{
		const int id = cacheId++;
		variable->cacheId = id;
		const int top = lua_gettop(L);
		lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME); // 1: cacheTable|nil
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1); //
			lua_newtable(L); // 1: {}
			lua_setfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME); //
			lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME); // 1: cacheTable
		}

		lua_pushvalue(L, valueIndex); // 1: cacheTable, 2: value

		// snprintf 性能不够，问题也很大，这里采用c++标准算法
		std::string key = std::to_string(id);
		lua_setfield(L, -2, key.c_str()); // 1: cacheTable

		lua_settop(L, top);
	}
}

// bool Debugger::HasCacheValue(int valueIndex) const
// {
// 	const int type = lua_type(L, valueIndex);
// 	if (type == LUA_TUSERDATA || type == LUA_TTABLE)
// 	{
// 		const int id = cacheId++;
// 		// ->cacheId = id;
// 		const int top = lua_gettop(L);
// 		lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME); // 1: cacheTable|nil
// 		if (lua_isnil(L, -1))
// 		{
// 			lua_pop(L, 1); //
// 			lua_newtable(L); // 1: {}
// 			lua_setfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME); //
// 			lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME); // 1: cacheTable
// 		}
// 		lua_pushvalue(L, valueIndex); // 1: cacheTable, 2: value
//
// 		// snprintf 性能不够，问题也很大，这里采用c++标准算法
// 		std::string Key = std::to_string(id);
// 		lua_setfield(L, -2, Key.c_str()); // 1: cacheTable
//
// 		lua_settop(L, top);
// 	}
// }

void Debugger::ClearCache() const
{
	if (!currentL)
	{
		return;
	}

	auto L = currentL;


	lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
	if (!lua_isnil(L, -1))
	{
		lua_pushnil(L);
		lua_setfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
	}
	lua_pop(L, 1);
}

void Debugger::DoAction(DebugAction action)
{
	// 锁加到这里
	std::lock_guard<std::mutex> lock(hookStateMtx);
	switch (action)
	{
	case DebugAction::Break:
		SetHookState(manager->stateBreak);
		break;
	case DebugAction::Continue:
		SetHookState(manager->stateContinue);
		break;
	case DebugAction::StepOver:
		SetHookState(manager->stateStepOver);
		break;
	case DebugAction::StepIn:
		SetHookState(manager->stateStepIn);
		break;
	case DebugAction::Stop:
		SetHookState(manager->stateStop);
		break;
	default: break;
	}
}

void Debugger::UpdateHook(int mask, lua_State* L)
{
	if (mask == 0)
		lua_sethook(L, nullptr, mask, 0);
	else
		lua_sethook(L, EmmyFacade::HookLua, mask, 0);
}


// _G.emmy.fixPath = function(path) return (newPath) end
int FixPath(lua_State* L)
{
	const auto path = lua_tostring(L, 1);
	lua_getglobal(L, "emmy");
	if (lua_istable(L, -1))
	{
		lua_getfield(L, -1, "fixPath");
		if (lua_isfunction(L, -1))
		{
			lua_pushstring(L, path);
			lua_call(L, 1, 1);
			return 1;
		}
	}
	return 0;
}

std::string Debugger::GetFile(lua_Debug* ar) const
{
	if (!currentL)
	{
		return "";
	}

	auto L = currentL;

	const char* file = getDebugSource(ar);
	if (getDebugCurrentLine(ar) < 0)
		return file;
	if (strlen(file) > 0 && file[0] == '@')
		file++;
	lua_pushcclosure(L, FixPath, 0);
	lua_pushstring(L, file);
	const int result = lua_pcall(L, 1, 1, 0);
	if (result == LUA_OK)
	{
		const auto p = lua_tostring(L, -1);
		lua_pop(L, 1);
		if (p)
		{
			return p;
		}
	}
	// todo: handle error
	return file;
}

void Debugger::HandleBreak()
{
	// to be on the safe side, hook it again
	UpdateHook(LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, currentL);

	if (EmmyFacade::Get().OnBreak(shared_from_this()))
	{
		EnterDebugMode();
	}
}

// host thread
void Debugger::EnterDebugMode()
{
	std::unique_lock<std::mutex> lock(runMtx);

	blocking = true;
	while (true)
	{
		std::unique_lock<std::mutex> lockEval(evalMtx);
		if (evalQueue.empty() && blocking)
		{
			lockEval.unlock();
			cvRun.wait(lock);
			lockEval.lock();
		}
		if (!evalQueue.empty())
		{
			const auto evalContext = evalQueue.front();
			evalQueue.pop();
			lockEval.unlock();
			const bool skip = skipHook;
			skipHook = true;
			evalContext->success = DoEval(evalContext);
			skipHook = skip;
			EmmyFacade::Get().OnEvalResult(evalContext);
			continue;
		}
		break;
	}
	ClearCache();
}

void Debugger::ExitDebugMode()
{
	blocking = false;
	cvRun.notify_all();
}


int EnvIndexFunction(lua_State* L)
{
	const int locals = lua_upvalueindex(1);
	const int upvalues = lua_upvalueindex(2);
	const char* name = lua_tostring(L, 2);
	// up value
	lua_getfield(L, upvalues, name);
	if (lua_isnil(L, -1) == 0)
	{
		return 1;
	}
	lua_pop(L, 1);
	// local value
	lua_getfield(L, locals, name);
	if (lua_isnil(L, -1) == 0)
	{
		return 1;
	}
	lua_pop(L, 1);
	// _ENV
	lua_getfield(L, upvalues, "_ENV");
	if (lua_istable(L, -1))
	{
		lua_getfield(L, -1, name); // _ENV[name]
		if (lua_isnil(L, -1) == 0)
		{
			return 1;
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	// global
	lua_getglobal(L, name);
	if (lua_isnil(L, -1) == 0)
	{
		return 1;
	}
	lua_pop(L, 1);
	return 0;
}

bool Debugger::CreateEnv(int stackLevel)
{
	if (!currentL)
	{
		return false;
	}

	auto L = currentL;


	//assert(L);
	//const auto L = L;

	lua_Debug ar{};
	if (!lua_getstack(L, stackLevel, &ar))
	{
		return false;
	}
	if (!lua_getinfo(L, "nSlu", &ar))
	{
		return false;
	}

	lua_newtable(L);
	const int env = lua_gettop(L);
	lua_newtable(L);
	const int envMetatable = lua_gettop(L);
	lua_newtable(L);
	const int locals = lua_gettop(L);
	lua_newtable(L);
	const int upvalues = lua_gettop(L);

	int idx = 1;
	// local values
	while (true)
	{
		const char* name = lua_getlocal(L, &ar, idx++);
		if (name == nullptr)
			break;
		if (name[0] == '(')
		{
			lua_pop(L, 1);
			continue;
		}
		lua_setfield(L, locals, name);
	}
	// up values
	if (lua_getinfo(L, "f", &ar))
	{
		const int fIdx = lua_gettop(L);
		idx = 1;
		while (true)
		{
			const char* name = lua_getupvalue(L, fIdx, idx++);
			if (name == nullptr)
				break;
			lua_setfield(L, upvalues, name);
		}
		lua_pop(L, 1);
	}
	int top = lua_gettop(L);
	assert(top == upvalues);

	// index function
	// up value: locals, upvalues
	lua_pushcclosure(L, EnvIndexFunction, 2);

	// envMetatable.__index = EnvIndexFunction
	lua_setfield(L, envMetatable, "__index");
	// setmetatable(env, envMetatable)
	lua_setmetatable(L, env);

	top = lua_gettop(L);
	assert(top == env);
	return true;
}

bool Debugger::ProcessBreakPoint(std::shared_ptr<BreakPoint> bp)
{
	if (!bp->condition.empty())
	{
		auto ctx = std::make_shared<EvalContext>();
		ctx->expr = bp->condition;
		ctx->depth = 1;
		bool suc = DoEval(ctx);
		return suc && ctx->result->valueType == LUA_TBOOLEAN && ctx->result->value == "true";
	}
	if (!bp->logMessage.empty())
	{
		DoLogMessage(bp);
		return false;
	}
	if (!bp->hitCondition.empty())
	{
		bp->hitCount++;

		return DoHitCondition(bp);
	}
	return true;
}

void Debugger::SetHookState(std::shared_ptr<HookState> newState)
{
	if (!currentL)
	{
		return;
	}

	auto L = currentL;

	hookState = nullptr;
	if (newState->Start(shared_from_this(), L))
	{
		hookState = newState;
	}
}

std::shared_ptr<EmmyDebuggerManager> Debugger::GetEmmyDebuggerManager()
{
	return manager;
}

int Debugger::GetStackLevel(bool skipC) const
{
	if (!currentL)
	{
		return 0;
	}

	auto L = currentL;

	int level = 0, i = 0;
	lua_Debug ar{};
	while (lua_getstack(L, i, &ar))
	{
		lua_getinfo(L, "l", &ar);
		if (getDebugCurrentLine(&ar) >= 0 || !skipC)
			level++;
		i++;
	}
	return level;
}

void Debugger::AsyncDoString(const std::string& code)
{
	doStringList.emplace_back(code);
}

void Debugger::CheckDoString()
{
	if (!currentL)
	{
		return;
	}

	auto L = currentL;


	if (!doStringList.empty())
	{
		const auto skip = skipHook;
		skipHook = true;
		const int t = lua_gettop(L);
		for (const auto& code : doStringList)
		{
			const int r = luaL_loadstring(L, code.c_str());
			if (r == LUA_OK)
			{
				lua_pcall(L, 0, 0, 0);
			}
			lua_settop(L, t);
		}
		skipHook = skip;
		assert(lua_gettop(L) == t);
		doStringList.clear();
	}
}

// message thread
bool Debugger::Eval(std::shared_ptr<EvalContext> evalContext, bool force)
{
	if (force)
		return DoEval(evalContext);
	if (!blocking)
	{
		return false;
	}
	// 加锁
	{
		std::unique_lock<std::mutex> lock(evalMtx);
		evalQueue.push(evalContext);
	}

	cvRun.notify_all();
	return true;
}

// host thread
bool Debugger::DoEval(std::shared_ptr<EvalContext> evalContext)
{
	if (!currentL || !evalContext)
	{
		return false;
	}

	auto L = currentL;

	//auto* const L = L;
	// From "cacheId"
	if (evalContext->cacheId > 0)
	{
		lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME); // 1: cacheTable|nil
		if (lua_type(L, -1) == LUA_TTABLE)
		{
			lua_getfield(L, -1, std::to_string(evalContext->cacheId).c_str()); // 1: cacheTable, 2: value
			GetVariable(evalContext->result, -1, evalContext->depth);
			lua_pop(L, 2);
			return true;
		}
		lua_pop(L, 1);
	}
	// LOAD AS "return expr"
	std::string statement = "return ";
	statement.append(evalContext->expr);
	int r = luaL_loadstring(L, statement.c_str());
	if (r == LUA_ERRSYNTAX)
	{
		evalContext->error = "syntax err: ";
		evalContext->error.append(evalContext->expr);
		return false;
	}
	// call
	const int fIdx = lua_gettop(L);
	// create env
	if (!CreateEnv(evalContext->stackLevel))
		return false;
	// setup env
#ifndef EMMY_USE_LUA_SOURCE
	lua_setfenv(L, fIdx);
#elif defined(EMMY_LUA_51) || defined(EMMY_LUA_JIT)
    lua_setfenv(L, fIdx);
#else //52 & 53
    lua_setupvalue(L, fIdx, 1);
#endif
	assert(lua_gettop(L) == fIdx);
	// call function() return expr end
	r = lua_pcall(L, 0, 1, 0);
	if (r == LUA_OK)
	{
		evalContext->result->name = evalContext->expr;
		GetVariable(evalContext->result, -1, evalContext->depth);
		lua_pop(L, 1);
		return true;
	}
	if (r == LUA_ERRRUN)
	{
		evalContext->error = lua_tostring(L, -1);
	}

	return false;
}

void Debugger::DoLogMessage(std::shared_ptr<BreakPoint> bp)
{
	std::string& logMessage = bp->logMessage;
	// 为什么不用regex?
	// 因为gcc 4.8 regex还是空实现
	// 而且后续版本的gcc中正则表达式行为似乎也不太正常
	enum class ParseState
	{
		Normal,
		LeftBrace,
		RightBrace
	} state = ParseState::Normal;

	std::vector<LogMessageReplaceExpress> replaceExpresses;


	std::size_t leftBraceBegin = 0;

	std::size_t rightBraceBegin = 0;

	// 如果在表达式中出现左大括号
	std::size_t exprLeftCount = 0;


	for (std::size_t index = 0; index != logMessage.size(); index++)
	{
		char ch = logMessage[index];

		switch (state)
		{
		case ParseState::Normal:
			{
				if (ch == '{')
				{
					state = ParseState::LeftBrace;
					leftBraceBegin = index;
					exprLeftCount = 0;
				}
				else if (ch == '}')
				{
					state = ParseState::RightBrace;
					rightBraceBegin = index;
				}
				break;
			}
		case ParseState::LeftBrace:
			{
				if (ch == '{')
				{
					// 认为是左双大括号转义为可见的'{'
					if (index == leftBraceBegin + 1)
					{
						replaceExpresses.emplace_back("{", leftBraceBegin, index, false);
						state = ParseState::Normal;
					}
					else
					{
						exprLeftCount++;
					}
				}
				else if (ch == '}')
				{
					// 认为是表达式内的大括号
					if (exprLeftCount > 0)
					{
						exprLeftCount--;
						continue;
					}

					replaceExpresses.emplace_back(logMessage.substr(leftBraceBegin + 1, index - leftBraceBegin - 1),
					                              leftBraceBegin, index, true);


					state = ParseState::Normal;
				}
				break;
			}
		case ParseState::RightBrace:
			{
				if (ch == '}' && (index == rightBraceBegin + 1))
				{
					replaceExpresses.emplace_back("}", rightBraceBegin, index, false);
				}
				else
				{
					//认为左右大括号失配，之前的不做处理，退格一位回去重新判断
					index--;
				}
				state = ParseState::Normal;
				break;
			}
		}
	}

	std::stringstream message;

	if (replaceExpresses.empty())
	{
		message << logMessage;
	}
	else
	{
		// 拼接字符串
		// 怎么replace 函数都没有啊

		std::size_t start = 0;
		for (std::size_t index = 0; index != replaceExpresses.size(); index++)
		{
			auto& replaceExpress = replaceExpresses[index];
			if (start < replaceExpress.StartIndex)
			{
				auto fragment = logMessage.substr(start, replaceExpress.StartIndex - start);
				message << fragment;
				start = replaceExpress.StartIndex;
			}

			if (replaceExpress.NeedEval)
			{
				auto ctx = std::make_shared<EvalContext>();
				ctx->expr = std::move(replaceExpress.Expr);
				ctx->depth = 1;
				bool succeed = DoEval(ctx);
				if (succeed)
				{
					message << ctx->result->value;
				}
				else
				{
					message << ctx->error;
				}
			}
			else
			{
				message << replaceExpress.Expr;
			}

			start = replaceExpress.EndIndex + 1;
		}

		if (start < logMessage.size())
		{
			auto fragment = logMessage.substr(start, logMessage.size() - start);
			message << fragment;
		}
	}

	std::string baseName = BaseName(bp->file);

	EmmyFacade::Get().SendLog(LogType::Info, "[%s:%d] %s", baseName.c_str(), bp->line, message.str().c_str());
}

std::shared_ptr<BreakPoint> Debugger::FindBreakPoint(lua_Debug* ar)
{
	if (!currentL)
	{
		return nullptr;
	}

	auto L = currentL;

	const int cl = getDebugCurrentLine(ar);
	auto lineSet = manager->GetLineSet();

	if (cl >= 0 && lineSet.find(cl) != lineSet.end())
	{
		lua_getinfo(L, "S", ar);
		const auto chunkname = GetFile(ar);
		return FindBreakPoint(chunkname, cl);
	}
	return nullptr;
}

std::shared_ptr<BreakPoint> Debugger::FindBreakPoint(const std::string& chunkname, int line)
{
	std::shared_ptr<BreakPoint> breakedpoint = nullptr;
	int maxMatchProcess = 0;

	auto breakpoints = manager->GetBreakpoints();
	for (const auto bp : breakpoints)
	{
		if (bp->line == line)
		{
			// fuzz match: bp(x/a/b/c), file(a/b/c)
			int matchProcess = FuzzyMatchFileName(chunkname, bp->file);

			if (matchProcess > 0 && matchProcess > maxMatchProcess)
			{
				maxMatchProcess = matchProcess;
				breakedpoint = bp;
			}
		}
	}

	return breakedpoint;
}

#undef min
bool Debugger::DoHitCondition(std::shared_ptr<BreakPoint> bp)
{
	auto& hitCondition = bp->hitCondition;

	enum class ParseState
	{
		ExpectedOperator,
		// 大于
		Gt,
		// 小于
		Le,
		// 单等号 
		Eq,

		ExpectedHitTimes,

		ParseDigit,

		ParseFinish
	} state = ParseState::ExpectedOperator;

	enum class Operator
	{
		// 大于
		Gt,
		// 小于
		Le,
		// 小于等于
		LeEq,
		// 大于等于
		GtEq,
		// 双等号
		EqEq,
	} evalOperator = Operator::EqEq;

	unsigned long long hitTimes = 0;

	for (std::size_t index = 0; index != hitCondition.size(); index++)
	{
		char ch = hitCondition[index];

		switch (state)
		{
		case ParseState::ExpectedOperator:
			{
				if (ch == ' ')
				{
					continue;
				}

				if (ch == '=')
				{
					state = ParseState::Eq;
				}
				else if (ch == '<')
				{
					state = ParseState::Le;
				}
				else if (ch == '>')
				{
					state = ParseState::Gt;
				}
				else
				{
					return false;
				}

				break;
			}
		case ParseState::Eq:
			{
				if (ch == '=')
				{
					evalOperator = Operator::EqEq;
					state = ParseState::ExpectedHitTimes;
				}
				else
				{
					return false;
				}
				break;
			}
		case ParseState::Gt:
			{
				if (ch == '=')
				{
					evalOperator = Operator::GtEq;
					state = ParseState::ExpectedHitTimes;
				}
				else if (isdigit(ch))
				{
					evalOperator = Operator::Gt;
					hitTimes = ch - '0';
					state = ParseState::ParseDigit;
				}
				else if (ch == ' ')
				{
					evalOperator = Operator::Gt;
					state = ParseState::ExpectedHitTimes;
				}
				else
				{
					return false;
				}
				break;
			}
		case ParseState::Le:
			{
				if (ch == '=')
				{
					evalOperator = Operator::LeEq;
					state = ParseState::ExpectedHitTimes;
				}
				else if (isdigit(ch))
				{
					evalOperator = Operator::Le;
					hitTimes = ch - '0';
					state = ParseState::ParseDigit;
				}
				else if (ch == ' ')
				{
					evalOperator = Operator::Gt;
					state = ParseState::ExpectedHitTimes;
				}
				else
				{
					return false;
				}
				break;
			}
		case ParseState::ExpectedHitTimes:
			{
				if (ch == ' ')
				{
					continue;
				}
				else if (isdigit(ch))
				{
					hitTimes = ch - '0';
					state = ParseState::ParseDigit;
				}
				else
				{
					return false;
				}
				break;
			}
		case ParseState::ParseDigit:
			{
				if (isdigit(ch))
				{
					hitTimes = hitTimes * 10 + (ch - '0');
				}
				else if (ch == ' ')
				{
					state = ParseState::ParseFinish;
				}
				else
				{
					return false;
				}

				break;
			}
		case ParseState::ParseFinish:
			{
				if (ch == ' ')
				{
					break;
				}
				else
				{
					return false;
				}
				break;
			}
		}
	}

	switch (evalOperator)
	{
	case Operator::EqEq:
		{
			return bp->hitCount == hitTimes;
		}
	case Operator::Gt:
		{
			return bp->hitCount > hitTimes;
		}
	case Operator::GtEq:
		{
			return bp->hitCount >= hitTimes;
		}
	case Operator::Le:
		{
			return bp->hitCount < hitTimes;
		}
	case Operator::LeEq:
		{
			return bp->hitCount <= hitTimes;
		}
	}


	return false;
}

// 重写模糊匹配算法
int Debugger::FuzzyMatchFileName(const std::string& chunkName, const std::string& fileName) const
{
	auto chunkSize = chunkName.size();
	auto fileSize = fileName.size();


	std::size_t chunkExtSize = 0;
	std::size_t fileExtSize = 0;
	// trim 掉后缀
	for (const auto& ext : manager->extNames)
	{
		if (EndWith(chunkName, ext))
		{
			if (ext.size() > chunkExtSize)
			{
				chunkExtSize = ext.size();
			}
		}

		if (EndWith(fileName, ext))
		{
			if (ext.size() > fileExtSize)
			{
				fileExtSize = ext.size();
			}
		}
	}

	chunkSize -= chunkExtSize;
	fileSize -= fileExtSize;

	// 我们用chunkname去匹配filename
	int maxMatchSize = static_cast<int>(std::min(chunkSize, fileSize));

	int matchProcess = 1;

	for (int i = 1; i != maxMatchSize; i++)
	{
		char cChar = chunkName[chunkSize - i];
		char fChar = fileName[fileSize - i];

		if (cChar != fChar)
		{
			// 以下匹配顺序是有意义的，不要轻易改变

			if (::tolower(cChar) == ::tolower(fChar))
			{
				continue;
			}

			// 认为来自编辑器的路径不会是相对路径
			// chunkname有可能是(./aaaa)也可能是(aaa/./bbb)
			// 并不匹配(../)的情况
			// 因为 ../的路径意义并不唯一
			if (cChar == '.')
			{
				std::size_t cLastindex = chunkSize - i + 1;

				if (cLastindex >= chunkSize)
				{
					matchProcess = 0;
					break;
				}

				char cLastChar = chunkName[cLastindex];
				if (cLastChar != '/' && cLastChar != '\\')
				{
					matchProcess = 0;
					break;
				}

				// 该值可能为负数
				int cNextIndex = static_cast<int>(chunkSize) - i - 1;
				if (cNextIndex < 0)
				{
					// 匹配已经完毕
					break;
				}

				char cNextChar = chunkName[cNextIndex];

				// 那chunkname 就是 aaa./bbbb 那就不匹配
				if (cNextChar != '/' && cNextChar != '\\')
				{
					matchProcess = 0;
					break;
				}

				// 这里会消耗掉 next的匹配
				i++;

				// 这里是指保持下一次循环时fChar不变
				fileSize += 2;
				continue;
			}

			if (cChar == '/' || cChar == '\\')
			{
				if (fChar == '/' || fChar == '\\')
				{
					matchProcess++;
					continue;
				}
				// 一些人会写出 require './aaaa' 这种代码
				// 导致lua程序 的chunkname 给的是 .\\/aaaa.lua


				//保持fChar 下次循环时不变
				fileSize++;
				continue;
			}

			// 那就是不匹配
			matchProcess = 0;
			break;
		}
	}

	return matchProcess;
}

void Debugger::ExecuteWithSkipHook(const Executor& exec)
{
	const bool skip = skipHook;
	skipHook = true;
	exec(currentL);
	skipHook = skip;
}

void Debugger::ExecuteOnLuaThread(const Executor& exec)
{
	std::unique_lock<std::mutex> lock(luaThreadMtx);
	luaThreadExecutors.push_back(exec);
}
