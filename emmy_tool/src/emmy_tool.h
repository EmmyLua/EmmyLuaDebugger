#pragma once
#include <Windows.h>
#include "command_line.h"

bool StartProcessAndInjectDll(LPCSTR exeFileName,
                              LPSTR command,
                              LPCSTR workDirectory,
                              LPCSTR dllDirectory,
                              LPCSTR dllName,
                              bool blockOnExit,
							  int debugPort,
							  bool createNewWindow
    );

bool InjectDll(DWORD processId, const char* dllDir, const char* dllFileName);
