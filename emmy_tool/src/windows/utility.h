#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <ImageHlp.h>
#include <iomanip>
#include <iostream>
#include "shared/shme.h"

struct ExeInfo {
	size_t			entryPoint;
	bool            managed;
	bool            i386;
};

bool GetExeInfo(LPCSTR fileName, ExeInfo&info);

/**
* Returns the top level window for the specified process. The first such window
* that's found is returned.
*/
HWND GetProcessWindow(DWORD processId);

struct Process {
	unsigned int    id;     // Windows process identifier
	std::string     name;   // Executable name
	std::string     title;  // Name from the main window of the process.
	std::string     path;   // Full path
	std::string     iconPath;// Icon path
};

/**
* Returns all of the processes on the machine that can be debugged.
*/
void GetProcesses(std::vector<Process>& processes);

int GetProcessByName(const char* name);

void SetBreakpoint(HANDLE hProcess, LPVOID entryPoint, bool set, BYTE *data);

bool InjectDllForProcess(HANDLE hProcess, const char *dllDir, const char *dllFileName);

bool InjectDll(DWORD processId, const char *dllDir, const char *dllFileName, bool capture);

void ReceiveLog(DWORD processId);

