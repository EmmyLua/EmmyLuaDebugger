#include <cstdio>
#include <string>
#include "emmy_tool.h"
#include "command_line.h"

int main(int argc, char **argv) {
	CommandLine commandLine;
	commandLine.AddTarget("attach");
	commandLine.AddTarget("list_processes");
	commandLine.AddTarget("arch_file", false);
	commandLine.AddTarget("arch_pid", false);
	commandLine.AddTarget("launch");
	commandLine.AddTarget("receive_log");

	// pid
	commandLine.Add<int>("p");
	// dir 
	commandLine.Add<std::string>("dir");
	// dll
	commandLine.Add<std::string>("dll");
	// exe
	commandLine.Add<std::string>("exe");
	// work space
	commandLine.Add<std::string>("work");
	// stop on process exit
	commandLine.Add<bool>("block-on-exit");
	commandLine.Add<int>("debug-port");
	commandLine.Add<bool>("listen-mode");
	commandLine.Add<bool>("create-new-window");
	// rest param
	commandLine.Add<std::string>("args", true);
	commandLine.Add<bool>("capture-log");
	commandLine.Add<bool>("unitbuf");

	if (!commandLine.Parse(argc, argv)) {
		printf("parse fail");
		return -1;
	}
	EmmyTool tool(commandLine);
	const std::string target = commandLine.GetTarget();
	if (target == "attach") {
		//emmy_tool.exe attach -p 100 -dir c:/xx -dll emmy_hook.dll
		return tool.Attach();
	}
	if (target == "list_processes") {
		//emmy_tool.exe list_processes
		return tool.ListProcesses();
	}
	if (target == "arch_file") {
		//emmy_tool.exe arch_file FILE_PATH
		return tool.ArchFile();
	}
	if (target == "arch_pid") {
		//emmy_tool.exe arch_pid PROCESS_ID
		return tool.ArchPid();
	}
	if (target == "launch") {
		//emmy_tool.exe run_and_attach -dir c:/xx -dll emmy_hook.dll -work d:fff/ -exe c:/lua/lua.exe -args test.lua
		return tool.Launch();
	}
	if (target == "receive_log") {
		return tool.ReceiveLog();
	}

	return -1;
}
