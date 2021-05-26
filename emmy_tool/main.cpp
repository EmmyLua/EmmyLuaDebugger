#include "emmy_tool.h"
#include <cstdio>
#include <iostream>
#include <string>
#include <Psapi.h>
#include "utility.h"
#include "command_line.h"

int doAttach(CommandLine& commandLine)
{
	const int pid = commandLine.Get<int>("p");
	std::string dir = commandLine.Get<std::string>("dir");
	std::string dll = commandLine.Get<std::string>("dll");
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

	std::string dlldir = commandLine.Get<std::string>("dir");
	std::string dll = commandLine.Get<std::string>("dll");
	std::string dir = commandLine.Get<std::string>("work");
	std::string exe = commandLine.Get<std::string>("exe");
	int debugPort = commandLine.Get<int>("debugPort");
	// 路径中可能存在空格
	std::string command = "\"" + exe + "\"" + " " + commandLine.Get<std::string>("args");
	bool blockOnExit = commandLine.Get<bool>("blockOnExit");
	bool createNewWindow = commandLine.Get<bool>("createNewWindow");
	
	if (!StartProcessAndInjectDll(exe.c_str(),
	                              const_cast<LPSTR>(command.c_str()),
	                              dir.c_str(),
	                              dlldir.c_str(),
	                              dll.c_str(),
	                              blockOnExit,
	                              debugPort,
								  createNewWindow
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
	commandLine.Add<int>("p");
	// dir 实际上是dll dir
	commandLine.Add<std::string>("dir");
	// dll 名字
	commandLine.Add<std::string>("dll");
	// exe 路径
	commandLine.Add<std::string>("exe");
	// work space
	commandLine.Add<std::string>("work");
	// stop on process exit
	commandLine.Add<bool>("blockOnExit");
	commandLine.Add<int>("debugPort");
	commandLine.Add<bool>("createNewWindow");
	// rest param
	commandLine.Add<std::string>("args", true);

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
