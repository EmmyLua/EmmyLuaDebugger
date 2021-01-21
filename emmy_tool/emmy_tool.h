#pragma once
#include <Windows.h>
#include "command_line.h"

bool StartProcessAndInjectDll(LPCSTR exeFileName,
                               LPSTR command,
                               LPCSTR directory,
                               PROCESS_INFORMATION& processInfo,
                               CommandLine& commandLine);

bool InjectDll(DWORD processId, const char* dllDir, const char* dllFileName);
