#include "emmy_tool.h"
#include <cstdio>
#include <string>
#include <Psapi.h>
#include "utility.h"

int doAttach(int argc, char** argv) {
	if (argc < 8) {
		printf("emmy_tool.exe attach -p 100 -dir c:/xx -dll emmy_hook.dll");
	}
	const int pid = atoi(argv[3]);
	const char* dir = argv[5];
	const char* dll = argv[7];
	if (!InjectDll(pid, dir, dll)) {
		return -1;
	}
	return 0;
}

int doListProcesses() {
	std::vector<Process> list;
	GetProcesses(list);

	for (auto& value : list) {
		printf("%d\n", value.id);
		printf("%s\n", value.title.c_str());
		printf("%s\n", value.path.c_str());
		printf("----\n");
	}
	return 0;
}

int doArchFile(const char* fileName) {
	ExeInfo info;
	if (GetExeInfo(fileName, info)) {
		printf("%d", info.i386);
		return info.i386;
	}
	return -1;//file not exist
}

int doArchPid(DWORD processId) {
	char fileName[_MAX_PATH];
	HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
	GetModuleFileNameEx(process, nullptr, fileName, _MAX_PATH);

	ExeInfo info;
	if (GetExeInfo(fileName, info)) {
		printf("%d", info.i386);
		return info.i386;
	}
	return -1;
}

int main(int argc, char** argv) {
	if (argc < 2)
		return -1;
	const std::string fn = argv[1];
	if (fn == "attach") {
		//emmy_tool.exe attach -p 100 -dir c:/xx -dll emmy_hook.dll
		return doAttach(argc, argv);
	}
	if (fn == "list_processes") {
		//emmy_tool.exe list_processes
		return doListProcesses();
	}
	if (fn == "arch_file") {
		//emmy_tool.exe arch_file FILE_PATH
		return doArchFile(argv[2]);
	}
	if (fn == "arch_pid") {
		//emmy_tool.exe arch_pid PROCESS_ID
		const DWORD processId = atoi(argv[2]);
		return doArchPid(processId);
	}
	return -1;
}