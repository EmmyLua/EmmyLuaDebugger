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
	using UniqueIdentifyType = unsigned long long;

	EmmyDebuggerManager();
	~EmmyDebuggerManager();

	/*
	 * 获得L 的main thread 所在的 debugger
	 */
	std::shared_ptr<Debugger> GetDebugger(lua_State* L);

	/*
	 * 如果L 是main thread 则添加一个新的debugger 否则返回他所在的main thread对应的debugger
	 */
	std::shared_ptr<Debugger> AddDebugger(lua_State* L);

	/*
	 * 从L 获取主lua state所在得debugger 移除并返回
	 */
	std::shared_ptr<Debugger> RemoveDebugger(lua_State* L);

	/*
	 * 获得所有得debugger
	 */
	std::vector<std::shared_ptr<Debugger>> GetDebuggers();

	void RemoveAllDebugger();
	/*
	 * 获得当前命中的debugger
	 */
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

	void SetRunning(bool value);

	bool IsRunning();

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
	UniqueIdentifyType GetUniqueIdentify(lua_State* L);

	// 需要一个锁，真的需要这个锁吗？
	std::mutex debuggerMtx;
	// key 是唯一标记（对普通lua就是main state指针，对luajit就是注册表指针）,value 是debugger
	std::map<UniqueIdentifyType , std::shared_ptr<Debugger>> debuggers;

	std::mutex breakDebuggerMtx;
	std::shared_ptr<Debugger> breakedDebugger;

	std::mutex breakpointsMtx;
	std::vector<std::shared_ptr<BreakPoint>> breakpoints;

	std::set<int> lineSet;

	std::mutex isRuningMtx;
	bool isRunning;
};
