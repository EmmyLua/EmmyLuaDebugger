#include "emmy_tool.h"
#include <cstdio>
#include <iostream>
#include <string>
#include <Psapi.h>
#include "utility.h"
#include "command_line.h"

int doAttach(CommandLine& commandLine)
{
	const int pid = std::stoi(commandLine.GetArg("p"));
	std::string dir = commandLine.GetArg("dir");
	std::string dll = commandLine.GetArg("dll");
	if (!InjectDll(pid, dir.c_str(), dll.c_str()))
	{
		return -1;
	}
	return 0;
}

int doListProcesses()
{
	std::vector<Process> list;
	GetProcesses(list);

	for (auto& value : list)
	{
		printf("%d\n", value.id);
		printf("%s\n", value.title.c_str());
		printf("%s\n", value.path.c_str());
		printf("----\n");
	}
	return 0;
}

int doArchFile(CommandLine& commandLine)
{
	ExeInfo info;
	if (GetExeInfo(commandLine.GetArg(2).c_str(), info))
	{
		printf("%d", info.i386);
		return info.i386;
	}
	return -1; //file not exist
}

int doArchPid(CommandLine& commandLine)
{
	const DWORD processId = std::stoi(commandLine.GetArg(2));
	char fileName[_MAX_PATH];
	HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
	GetModuleFileNameEx(process, nullptr, fileName, _MAX_PATH);

	ExeInfo info;
	if (GetExeInfo(fileName, info))
	{
		printf("%d", info.i386);
		return info.i386;
	}
	return -1;
}

int doRunAndAttach(CommandLine& commandLine)
{
	// printf("emmy_tool.exe run_and_attach -dir c:/xx -dll emmy_hook.dll -work d:fff/ -exe c:/lua/lua.exe -args test.lua");

	std::string dlldir = commandLine.GetArg("dir");
	std::string dll = commandLine.GetArg("dll");
	std::string dir = commandLine.GetArg("work");
	std::string exe = commandLine.GetArg("exe");
	std::string command = exe + " " + commandLine.GetArg("args");

	if (!StartProcessAndInjectDll(exe.c_str(),
	                              const_cast<LPSTR>(command.c_str()),
	                              dir.c_str(),
	                              dlldir.c_str(),
	                              dll.c_str(),
	                              true
	))
	{
		return -1;
	}

	return 0;
}

int main(int argc, char** argv)
{
	CommandLine commandLine;
	commandLine.AddTarget("attach");
	commandLine.AddTarget("list_processes");
	commandLine.AddTarget("arch_file", false);
	commandLine.AddTarget("arch_pid", false);
	commandLine.AddTarget("run_and_attach");

	// pid
	commandLine.AddArg("p");
	// dir 实际上是dll dir
	commandLine.AddArg("dir");
	// dll 名字
	commandLine.AddArg("dll");
	//exe 路径
	commandLine.AddArg("exe");
	// work space
	commandLine.AddArg("work");
	// rest param
	commandLine.AddArg("args", true);

	if (!commandLine.Parse(argc, argv))
	{
		return -1;
	}

	const std::string target = commandLine.GetTarget();
	if (target == "attach")
	{
		//emmy_tool.exe attach -p 100 -dir c:/xx -dll emmy_hook.dll
		return doAttach(commandLine);
	}
	if (target == "list_processes")
	{
		//emmy_tool.exe list_processes
		return doListProcesses();
	}
	if (target == "arch_file")
	{
		//emmy_tool.exe arch_file FILE_PATH
		return doArchFile(commandLine);
	}
	if (target == "arch_pid")
	{
		//emmy_tool.exe arch_pid PROCESS_ID
		return doArchPid(commandLine);
	}
	if (target == "run_and_attach")
	{
		//emmy_tool.exe run_and_attach -dir c:/xx -dll emmy_hook.dll -work d:fff/ -exe c:/lua/lua.exe -args test.lua
		return doRunAndAttach(commandLine);
	}
	return -1;
}
