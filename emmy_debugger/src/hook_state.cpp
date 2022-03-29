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
#include "emmy_debugger/hook_state.h"
#include "emmy_debugger/emmy_debugger.h"
#include "emmy_debugger/api/lua_api.h"
#include "emmy_debugger/emmy_facade.h"

HookState::HookState():
	currentStateL(nullptr)
{
}

bool HookState::Start(std::shared_ptr<Debugger> debugger, lua_State* current)
{
	currentStateL = current;
	return true;
}

void HookState::ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar)
{
}

bool HookStateContinue::Start(std::shared_ptr<Debugger> debugger, lua_State* current)
{
	debugger->ExitDebugMode();
	return true;
}

bool StackLevelBasedState::Start(std::shared_ptr<Debugger> debugger, lua_State* current)
{
	if (current == nullptr)
		return false;
	currentStateL = current;
	oriStackLevel = newStackLevel = debugger->GetStackLevel(false);
	return true;
}

void StackLevelBasedState::UpdateStackLevel(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar)
{
	if (L != currentStateL)
	{
		return;
	}
	// this is needed to check if the stack got shorter or longer.
	// unfortunately counting call/return calls is not reliable.
	// the discrepancy may happen when "pcall(load, '')" call is made
	// or when "error()" is called in a function.
	// in either case there are more "call" than "return" events reported.
	// this validation is done for every "line" event, but should be "cheap"
	// as it checks for the stack to get shorter (or longer by one call).
	// start from one level higher just in case we need to grow the stack.
	// this may happen after coroutine.resume call to a function that doesn't
	// have any other instructions to execute. it triggers three returns:
	// "return, tail return, return", which needs to be accounted for.

	newStackLevel = debugger->GetStackLevel(true);

	// "cheap" version
	lua_Debug ar2{};
	for (int i = newStackLevel + 1; i >= 0; --i)
	{
		if (lua_getstack(L, i, &ar2))
		{
			newStackLevel = i + 1;
			break;
		}
	}
}

bool HookStateStepIn::Start(std::shared_ptr<Debugger> debugger, lua_State* current)
{
	if (!StackLevelBasedState::Start(debugger, current))
		return false;
	lua_Debug ar{};
	lua_getstack(current, 0, &ar);
	lua_getinfo(current, "nSl", &ar);
	file = getDebugSource(&ar);
	line = getDebugCurrentLine(&ar);
	debugger->ExitDebugMode();
	return true;
}

void HookStateStepIn::ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar)
{
	UpdateStackLevel(debugger, L, ar);
	if (getDebugEvent(ar) == LUA_HOOKLINE)
	{
		auto currentLine = getDebugCurrentLine(ar);
		auto source = getDebugSource(ar);
		if(currentLine != line || file != source)
		{
			debugger->HandleBreak();
		}
		return;
	}

	StackLevelBasedState::ProcessHook(debugger, L, ar);
}

bool HookStateStepOut::Start(std::shared_ptr<Debugger> debugger, lua_State* current)
{
	if (!StackLevelBasedState::Start(debugger, current))
		return false;
	debugger->ExitDebugMode();
	return true;
}

void HookStateStepOut::ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar)
{
	UpdateStackLevel(debugger, L, ar);
	if (newStackLevel < oriStackLevel)
	{
		debugger->HandleBreak();
		return;
	}
	StackLevelBasedState::ProcessHook(debugger, L, ar);
}

bool HookStateStepOver::Start(std::shared_ptr<Debugger> debugger, lua_State* current)
{
	if (!StackLevelBasedState::Start(debugger, current))
		return false;
	lua_Debug ar{};
	lua_getstack(current, 0, &ar);
	lua_getinfo(current, "nSl", &ar);
	file = getDebugSource(&ar);
	line = getDebugCurrentLine(&ar);
	debugger->ExitDebugMode();
	return true;
}

void HookStateStepOver::ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar)
{
	UpdateStackLevel(debugger, L, ar);
	// step out
	if (newStackLevel < oriStackLevel)
	{
		debugger->HandleBreak();
		return;
	}


	if (getDebugEvent(ar) == LUA_HOOKLINE &&
		getDebugCurrentLine(ar) != line &&
		newStackLevel == oriStackLevel)
	{
		lua_getinfo(L, "Sl", ar);

		if (getDebugSource(ar) == file || line == -1)
		{
			debugger->HandleBreak();
			return;
		}
	}
	StackLevelBasedState::ProcessHook(debugger, L, ar);
}

void HookStateBreak::ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar)
{
	if (getDebugEvent(ar) == LUA_HOOKLINE)
	{
		debugger->HandleBreak();
	}
	else
	{
		HookState::ProcessHook(debugger, L, ar);
	}
}

bool HookStateStop::Start(std::shared_ptr<Debugger> debugger, lua_State* current)
{
	if (current == nullptr)
		return false;

	// 不能取消当前hook
	// 当unity 停止play之后附加调试依然运行，此时lua虚拟机可能已经销毁
	// 所以以下操作会导致进程崩溃
	// debugger->UpdateHook(0, current);
	
	// 此处会引发递归加锁而报错，而如果使用递归锁对调试体验影响
	// debugger->DoAction(DebugAction::Continue);
	debugger->SetHookState(debugger->GetEmmyDebuggerManager()->stateContinue);

	return true;
}
