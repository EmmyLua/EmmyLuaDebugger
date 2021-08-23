#pragma once

#include <memory>
#include <map>
#include <set>
#include "emmy_debugger/hook_state.h"
#include "emmy_debugger/emmy_debugger.h"
#include "emmy_debugger/emmy_helper.h"
#include "api/lua_api.h"


class EmmyDebuggerManager : public std::enable_shared_from_this<EmmyDebuggerManager>
{
public:
	EmmyDebuggerManager();
	~EmmyDebuggerManager();

	std::shared_ptr<Debugger> GetDebugger(lua_State* L);

	std::shared_ptr<Debugger> AddDebugger(lua_State* L);

	std::shared_ptr<Debugger> RemoveDebugger(lua_State* L);

	std::vector<std::shared_ptr<Debugger>> GetDebuggers();

	std::shared_ptr<Debugger> GetBreakedpoint();

	void SetBreakedDebugger(std::shared_ptr<Debugger> debugger);

	bool IsDebuggerEmpty();

	void AddBreakpoint(std::shared_ptr<BreakPoint> breakpoint);
	// 返回拷贝后的断点列表
	std::vector<std::shared_ptr<BreakPoint>> GetBreakpoints();
	void RemoveBreakpoint(const std::string& file, int line);
	void RemoveAllBreakPoints();
	void RefreshLineSet();
	// 返回拷贝后的断点行集
	std::set<int> GetLineSet();

	void HandleBreak(lua_State* L);

	// 响应行为
	void DoAction(DebugAction action);

	// 计算表达式
	void Eval(std::shared_ptr<EvalContext> ctx);

	void OnDisconnect();

	// public 成员放下面
	std::shared_ptr<HookStateBreak> stateBreak;
	std::shared_ptr<HookStateStepOver> stateStepOver;
	std::shared_ptr<HookStateStepIn> stateStepIn;
	std::shared_ptr<HookStateStepOut> stateStepOut;
	std::shared_ptr<HookStateContinue> stateContinue;
	std::shared_ptr<HookStateStop> stateStop;
	// 按道理需要加锁
	// 但实际上通常不会改变
	// 暂时不加
	std::string helperCode;
	std::vector<std::string> extNames;
private:
	// 如果其他lua虚拟机在不同线程，则需要一个锁
	std::mutex debuggerMtx;
	std::map<lua_State*, std::shared_ptr<Debugger>> debuggers;

	std::mutex breakDebuggerMtx;
	std::shared_ptr<Debugger> breakedDebugger;

	std::mutex breakpointsMtx;
	std::vector<std::shared_ptr<BreakPoint>> breakpoints;
	std::set<int> lineSet;
};
