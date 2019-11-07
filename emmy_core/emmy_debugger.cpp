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
#include "emmy_debugger.h"
#include "emmy_core.h"
#include "emmy_facade.h"
#include "hook_state.h"
#include <algorithm>

bool query_variable(Variable* variable, lua_State* L, const char* typeName, int object, int depth);

#define CACHE_TABLE_NAME "_emmy_cache_table_"

int cacheId = 1;
int cacheLine = -1;
const char* cacheSource = nullptr;

void HookLua(lua_State* L, lua_Debug* ar) {
	Debugger::Get()->Hook(L, ar);
}

Debugger* Debugger::Get() {
	static Debugger instance;
	return &instance;
}

Debugger::Debugger():
	currentStateL(nullptr),
	hookState(nullptr),
	running(false),
	skipHook(false),
	blocking(false) {
	stateBreak = new HookStateBreak();
	stateContinue = new HookStateContinue();
	stateStepOver = new HookStateStepOver();
	stateStop = new HookStateStop();
	stateStepIn = new HookStateStepIn();
	stateStepOut = new HookStateStepOut();
}

Debugger::~Debugger() {
	for (auto bp : breakPoints) {
		delete bp;
	}
	breakPoints.clear();
	delete stateBreak;
	stateBreak = nullptr;
	delete stateContinue;
	stateContinue = nullptr;
	delete stateStepOver;
	stateStepOver = nullptr;
	delete stateStop;
	stateStop = nullptr;
	delete stateStepIn;
	stateStepIn = nullptr;
	delete stateStepOut;
	stateStepOut = nullptr;
}

void Debugger::Start(std::string & code) {
	helperCode = code;
	skipHook = false;
	blocking = false;
	running = true;
	doStringList.clear();
	states.clear();
	RemoveAllBreakpoints();
}

void Debugger::Attach(lua_State* L) {
	if (!running)
		return;
	states.insert(L);
	// execute helper code
	if (!helperCode.empty()) {
		const int t = lua_gettop(L);
		const int r = luaL_loadstring(L, helperCode.c_str());
		if (r == LUA_OK) {
			lua_pcall(L, 0, 0, 0);
		}
		lua_settop(L, t);
	}
	// todo: just set hook when break point added.
	UpdateHook(L, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET);

	// todo: hook co
	// auto root = G(L)->allgc;
}

void Debugger::Detach(lua_State* L) {
	const auto it = states.find(L);
	if (it != states.end()) {
		states.erase(it);
	}
}

void Debugger::Hook(lua_State* L, lua_Debug* ar) {
	if (skipHook) {
		return;
	}
	if (ar->event == LUA_HOOKLINE 
		&& (cacheLine != ar->currentline || cacheSource != ar->source)) {
		cacheSource = ar->source;
		cacheLine = ar->currentline;
		const auto bp = FindBreakPoint(L, ar);
		if (bp) {
			HandleBreak(L);
			return;
		}
		if (hookState)
			hookState->ProcessHook(this, L, ar);
	}
}

void Debugger::Stop() {
	running = false;
	skipHook = true;
	blocking = false;
	for (auto L : states) {
		UpdateHook(L, 0);
	}
	states.clear();
	hookState = nullptr;
	ExitDebugMode();
}

bool Debugger::IsRunning() const {
	return running;
}

bool Debugger::GetStacks(lua_State* L, std::vector<Stack*>& stacks, StackAllocatorCB alloc) {
	int level = 0;
	while (true) {
		lua_Debug ar{};
		if (!lua_getstack(L, level, &ar)) {
			break;
		}
		if (!lua_getinfo(L, "nSlu", &ar)) {
			continue;
		}
		auto stack = alloc();
		stack->file = GetFile(L, &ar);
		stack->functionName = ar.name == nullptr ? "" : ar.name;
		stack->level = level;
		stack->line = ar.currentline;
		stacks.push_back(stack);
		// get variables
		{
			for (int i = 1;; i++) {
				const char* name = lua_getlocal(L, &ar, i);
				if (name == nullptr) {
					break;
				}
				if (name[0] == '(') {
					lua_pop(L, 1);
					continue;
				}

				// add local variable
				auto var = stack->CreateVariable();
				var->name = name;
				GetVariable(var, L, -1, 1);
				stack->localVariables.push_back(var);
			}

			if (lua_getinfo(L, "f", &ar)) {
				const int fIdx = lua_gettop(L);
				for (int i = 1;; i++) {
					const char* name = lua_getupvalue(L, fIdx, i);
					if (!name) {
						break;
					}

					// add up variable
					auto var = stack->CreateVariable();
					var->name = name;
					GetVariable(var, L, -1, 1);
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

bool CallMetaFunction(lua_State* L, int valueIndex, const char* method, int numResults, int& result) {
	if (lua_getmetatable(L, valueIndex)) {
		const int metaIndex = lua_gettop(L);
		if (!lua_isnil(L, metaIndex)) {
			lua_pushstring(L, method);
			lua_rawget(L, metaIndex);
			if (lua_isnil(L, -1)) {
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

void Debugger::GetVariable(Variable* variable, lua_State* L, int index, int depth, bool queryHelper) {
	const int t1 = lua_gettop(L);
	index = lua_absindex(L, index);
	CacheValue(L, index, variable);
	const int type = lua_type(L, index);
	const char* typeName = lua_typename(L, type);
	variable->valueTypeName = typeName;
	variable->valueType = type;
	if (queryHelper && (type == LUA_TTABLE || type == LUA_TUSERDATA)) {
		if (query_variable(variable, L, typeName, index, depth)) {
			return;
		}
	}
	switch (type) {
	case LUA_TNIL: {
		variable->value = "nil";
		break;
	}
	case LUA_TNUMBER: {
		variable->value = lua_tostring(L, index);
		break;
	}
	case LUA_TBOOLEAN: {
		const bool v = lua_toboolean(L, index);
		variable->value = v ? "true" : "false";
		break;
	}
	case LUA_TSTRING: {
		variable->value = lua_tostring(L, index);
		break;
	}
	case LUA_TFUNCTION: {
		void* fAddr = lua_topointer(L, index);
		char buff[100];
		snprintf(buff, sizeof(buff), "%p", fAddr);
		variable->value = buff;
		break;
	}
	case LUA_TUSERDATA: {
		auto string = lua_tostring(L, index);
		if (string == nullptr) {
			int result;
			if (CallMetaFunction(L, t1, "__tostring", 1, result) && result == 0) {
				string = lua_tostring(L, -1);
				lua_pop(L, 1);
			}
		}
		if (string) {
			variable->value = string;
		}
		else {
			void* fAddr = lua_topointer(L, index);
			char buff[100];
			snprintf(buff, sizeof(buff), "%p", fAddr);
			variable->value = buff;
		}
		if (depth > 1) {
			if (lua_getmetatable(L, index)) {
				GetVariable(variable, L, -1, depth);
				lua_pop(L, 1); //pop meta
			}
		}
		break;
	}
	case LUA_TLIGHTUSERDATA: {
		void* fAddr = lua_topointer(L, index);
		char buff[100];
		snprintf(buff, sizeof(buff), "%p", fAddr);
		variable->value = buff;
		break;
	}
	case LUA_TTHREAD: {
		void* fAddr = lua_topointer(L, index);
		char buff[100];
		snprintf(buff, sizeof(buff), "%p", fAddr);
		variable->value = buff;
		break;
	}
	case LUA_TTABLE: {
		int tableSize = 0;
		void* tableAddr = lua_topointer(L, index);
		lua_pushnil(L);
		while (lua_next(L, index)) {
			// k: -2, v: -1
			if (depth > 1) {
				//todo: use allocator
				const auto v = new Variable();
				const auto t = lua_type(L, -2);
				v->nameType = t;
				if (t == LUA_TSTRING) {
					v->name = lua_tostring(L, -2);
				}
				else if (t == LUA_TNUMBER) {
					lua_pushvalue(L, -2); // avoid error: "invalid key to 'next'" ???
					v->name = lua_tostring(L, -1);
					lua_pop(L, 1);
				}
				else {
					v->name = lua_typename(L, t);
				}
				GetVariable(v, L, -1, depth - 1);
				variable->children.push_back(v);
			}
			lua_pop(L, 1);
			tableSize++;
		}
		char buff[100];
		snprintf(buff, sizeof(buff), "table(%p)", tableAddr);
		variable->value = buff;
		break;
	}
	}
	const int t2 = lua_gettop(L);
	assert(t2 == t1);
}

void Debugger::CacheValue(lua_State* L, int valueIndex, Variable* variable) const {
	const int type = lua_type(L, valueIndex);
	if (type == LUA_TUSERDATA || type == LUA_TTABLE) {
		const int id = cacheId++;
		variable->cacheId = id;
		const int t1 = lua_gettop(L);
		lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);		// 1: cacheTable|nil
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);											//
			lua_newtable(L);										// 1: {}
			lua_setfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);	//
			lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);	// 1: cacheTable
		}
		lua_pushvalue(L, valueIndex);								// 1: cacheTable, 2: value
		char Key[10];
		sprintf(Key, "%d", id);
		lua_setfield(L, -2, Key);									// 1: cacheTable
		lua_settop(L, t1);
	}
}

void Debugger::ClearCache(lua_State* L) const {
	lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
	if (!lua_isnil(L, -1)) {
		lua_pushnil(L);
		lua_setfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
	}
	lua_pop(L, 1);
}

void Debugger::DoAction(DebugAction action) {
	const auto L = currentStateL;
	switch (action) {
	case DebugAction::Break:
		SetHookState(L, stateBreak);
		break;
	case DebugAction::Continue:
		SetHookState(L, stateContinue);
		break;
	case DebugAction::StepOver:
		SetHookState(L, stateStepOver);
		break;
	case DebugAction::StepIn:
		SetHookState(L, stateStepIn);
		break;
	case DebugAction::StepOut:
		SetHookState(L, stateStepOut);
		break;
	case DebugAction::Stop:
		SetHookState(L, stateStop);
		break;
	default: break;
	}
}

void Debugger::UpdateHook(lua_State* L, int mask) {
	if (mask == 0)
		lua_sethook(L, nullptr, mask, 0);
	else
		lua_sethook(L, HookLua, mask, 0);
}

// _G.emmy.fixPath = function(path) return (newPath) end
int FixPath(lua_State* L) {
	const auto path = lua_tostring(L, 1);
	lua_getglobal(L, "emmy");
	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, "fixPath");
		if (lua_isfunction(L, -1)) {
			lua_pushstring(L, path);
			lua_call(L, 1, 1);
			return 1;
		}
	}
	return 0;
}

std::string& replace_str(std::string& str, const std::string& to_replaced, const std::string& newchars)
{
	for (std::string::size_type pos(0); pos != std::string::npos; pos += newchars.length())
	{
		pos = str.find(to_replaced, pos);
		if (pos != std::string::npos)
			str.replace(pos, to_replaced.length(), newchars);
		else
			break;
	}
	return str;
}

std::string Debugger::GetFile(lua_State* L, lua_Debug* ar) const {
	assert(L);
	const char* file = ar->source;
	do {
		if (ar->currentline < 0)
			break;
		if (strlen(file) > 0 && file[0] == '@')
			file++;

		lua_pushcclosure(L, FixPath, 0);
		lua_pushstring(L, file);
		const int result = lua_pcall(L, 1, 1, 0);
		if (result == LUA_OK) {
			const auto p = lua_tostring(L, -1);
			lua_pop(L, 1);
			if (p) {
				file = p;
				break;
			}
		}
	} while (false);

	std::string str = std::string(file);

	for (const auto& ext : extNames) {
		if (str.find(ext) > 0) {
			replace_str(str, ext, "");
		}
	}
	replace_str(str, ".", "/");

	// todo: handle error
	return str;
}

void Debugger::HandleBreak(lua_State* L) {
	currentStateL = L;
	// to be on the safe side, hook it again
	UpdateHook(L, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET);
	EmmyFacade::Get()->OnBreak(L);
	EnterDebugMode(L);
}

// host thread
void Debugger::EnterDebugMode(lua_State* L) {
	std::unique_lock<std::mutex> lock(mutexRun);
	blocking = true;
	while (true) {
		std::unique_lock<std::mutex> lockEval(mutexEval);
		if (evalQueue.empty() && blocking) {
			lockEval.unlock();
			cvRun.wait(lock);
			lockEval.lock();
		}
		if (!evalQueue.empty()) {
			const auto evalContext = evalQueue.front();
			evalQueue.pop();
			lockEval.unlock();
			const bool skip = skipHook;
			skipHook = true;
			evalContext->success = DoEval(evalContext);
			skipHook = skip;
			EmmyFacade::Get()->OnEvalResult(evalContext);
			continue;
		}
		break;
	}
	ClearCache(L);
}

void Debugger::ExitDebugMode() {
	blocking = false;
	cvRun.notify_all();
}

void Debugger::ParsePathParts(const std::string& file, std::vector<std::string>& paths) {
	size_t idx = 0;

	for (size_t i = 0; i < file.length(); i++) {
		const char c = file.at(i);
		if (c == '/' || c == '\\') {
			const auto part = file.substr(idx, i - idx);
			idx = i + 1;
			// ../a/b/c
			if (part == "..") {
				if (!paths.empty()) paths.pop_back();
				continue;
			}
			// ./a/b/c
			if ((part == "." || part.empty()) && paths.empty()) {
				continue;
			}
			paths.emplace_back(part);
		}
	}
	// file name
	paths.emplace_back(file.substr(idx));
}

void Debugger::AddBreakPoint(const BreakPoint& breakPoint) {
	std::lock_guard <std::mutex> lock(mutexBP);
	const auto bp = new BreakPoint();
	bp->file = breakPoint.file;
	std::transform(bp->file.begin(), bp->file.end(), bp->file.begin(), tolower);
	bp->condition = breakPoint.condition;
	bp->line = breakPoint.line;
	ParsePathParts(bp->file, bp->pathParts);
	breakPoints.push_back(bp);
	RefreshLineSet();
}

void Debugger::RemoveBreakPoint(const std::string& file, int line) {
	std::string lowerCaseFile = file;
	std::transform(file.begin(), file.end(), lowerCaseFile.begin(), tolower);
	std::lock_guard <std::mutex> lock(mutexBP);
	auto it = breakPoints.begin();
	while (it != breakPoints.end()) {
		const auto bp = *it;
		if (bp->file == lowerCaseFile && bp->line == line) {
			breakPoints.erase(it);
			delete bp;
			break;
		}
		++it;
	}
	RefreshLineSet();
}

void Debugger::RemoveAllBreakpoints() {
	lineSet.clear();
	breakPoints.clear();
}

int EnvIndexFunction(lua_State* L) {
	const int locals = lua_upvalueindex(1);
	const int upvalues = lua_upvalueindex(2);
	const char* name = lua_tostring(L, 2);
	// up value
	lua_getfield(L, upvalues, name);
	if (lua_isnil(L, -1) == 0) {
		return 1;
	}
	lua_pop(L, 1);
	// local value
	lua_getfield(L, locals, name);
	if (lua_isnil(L, -1) == 0) {
		return 1;
	}
	lua_pop(L, 1);
	// global
	lua_getglobal(L, name);
	if (lua_isnil(L, -1) == 0) {
		return 1;
	}
	lua_pop(L, 1);
	return 0;
}

bool Debugger::CreateEnv(int stackLevel) {
	assert(currentStateL);
	const auto L = currentStateL;

	lua_Debug ar{};
	if (!lua_getstack(L, stackLevel, &ar)) {
		return false;
	}
	if (!lua_getinfo(L, "nSlu", &ar)) {
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
	while (true) {
		const char* name = lua_getlocal(L, &ar, idx++);
		if (name == nullptr)
			break;
		if (name[0] == '(') {
			lua_pop(L, 1);
			continue;
		}
		lua_setfield(L, locals, name);
	}
	// up values
	if (lua_getinfo(L, "f", &ar)) {
		const int fIdx = lua_gettop(L);
		idx = 1;
		while (true) {
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

void Debugger::SetHookState(lua_State* L, HookState* newState) {
	hookState = nullptr;
	if (newState->Start(this, L)) {
		hookState = newState;
	}
}

int Debugger::GetStackLevel(lua_State* L, bool skipC) const {
	int level = 0, i = 0;
	lua_Debug ar{};
	while (lua_getstack(L, i, &ar)) {
		lua_getinfo(L, "l", &ar);
		if (ar.currentline >= 0 || !skipC)
			level++;
		i++;
	}
	return level;
}

void Debugger::AsyncDoString(const char* code) {
	doStringList.emplace_back(code);
}

void Debugger::CheckDoString(lua_State* L) {
	if (!doStringList.empty()) {
		const auto skip = skipHook;
		skipHook = true;
		const int t = lua_gettop(L);
		for (const auto& code : doStringList) {
			const int r = luaL_loadstring(L, code.c_str());
			if (r == LUA_OK) {
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
bool Debugger::Eval(EvalContext* evalContext, bool force) {
	if (force)
		return DoEval(evalContext);
	if (!blocking)
		return false;
	std::unique_lock<std::mutex> lock(mutexEval);
	evalQueue.push(evalContext);
	lock.unlock();
	cvRun.notify_all();
	return true;
}

// host thread
bool Debugger::DoEval(EvalContext* evalContext) {
	assert(currentStateL);
	assert(evalContext);
	const auto L = currentStateL;
	// From "cacheId"
	if (evalContext->cacheId > 0) {
		lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);	// 1: cacheTable|nil
		if (lua_type(L, -1) == LUA_TTABLE) {
			char Key[10];
			sprintf(Key, "%d", evalContext->cacheId);
			lua_getfield(L, -1, Key);							// 1: cacheTable, 2: value
			GetVariable(&evalContext->result, L, -1, evalContext->depth);
			lua_pop(L, 2);
			return true;
		}
		lua_pop(L, 1);
	}
	// LOAD AS "return expr"
	std::string statement = "return ";
	statement.append(evalContext->expr);
	int r = luaL_loadstring(L, statement.c_str());
	if (r == LUA_ERRSYNTAX) {
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
#elif EMMY_LUA_51
    lua_setfenv(L, fIdx);
#else //52 & 53
    lua_setupvalue(L, fIdx, 1);
#endif
	assert(lua_gettop(L) == fIdx);
	// call function() return expr end
	r = lua_pcall(L, 0, 1, 0);
	if (r == LUA_OK) {
		evalContext->result.name = evalContext->expr;
		GetVariable(&evalContext->result, L, -1, evalContext->depth);
		return true;
	}
	if (r == LUA_ERRRUN) {
		evalContext->error = lua_tostring(L, -1);
	}

	return false;
}

BreakPoint* Debugger::FindBreakPoint(lua_State* L, lua_Debug* ar) {
	if (ar->currentline >= 0 && lineSet.find(ar->currentline) != lineSet.end()) {
		lua_getinfo(L, "S", ar);
		const auto file = GetFile(L, ar);
		return FindBreakPoint(file, ar->currentline);
	}
	return nullptr;
}

BreakPoint* Debugger::FindBreakPoint(const std::string& file, int line) {
	std::lock_guard <std::mutex> lock(mutexBP);
	std::vector<std::string> pathParts;
	std::string lowerCaseFile = file;
	std::transform(file.begin(), file.end(), lowerCaseFile.begin(), tolower);
	ParsePathParts(lowerCaseFile, pathParts);
	auto it = breakPoints.begin();
	while (it != breakPoints.end()) {
		const auto bp = *it;
		if (bp->line == line) {
			// full match: bp(a/b/c), file(a/b/c)
			if (bp->file == lowerCaseFile) {
				return *it;
			}
			// fuzz match: bp(x/a/b/c), file(a/b/c)
			if (bp->pathParts.size() >= pathParts.size() && MatchFileName(pathParts.back(), bp->pathParts.back())) {
				bool match = true;
				for (size_t i = 1; i < pathParts.size(); i++) {
					const auto p = *(bp->pathParts.end() - i - 1);
					const auto f = *(pathParts.end() - i - 1);
					if (p != f) {
						match = false;
						break;
					}
				}
				if (match) {
					return bp;
				}
			}
		}
		++it;
	}

	return nullptr;
}

bool Debugger::MatchFileName(const std::string& chunkName, const std::string& fileName) const {
	if (chunkName == fileName)
		return true;
	std::string pureFileName = fileName.substr(0, fileName.find('.'));
	if (chunkName == pureFileName)
		return true;
	// abc == abc.lua
	for (const auto& ext : extNames) {
		if (chunkName + ext == fileName) {
			return true;
		}
	}
	return false;
}


void Debugger::RefreshLineSet() {
	lineSet.clear();
	for (auto bp : breakPoints) {
		lineSet.insert(bp->line);
	}
}

void Debugger::ExecuteWithSkipHook(Executor exec) {
	const bool skip = skipHook;
	skipHook = true;
	exec();
	skipHook = skip;
}

void Debugger::SetExtNames(const std::vector<std::string>& names) {
	this->extNames = names;
}
