#pragma once
#include <Windows.h>

bool StartProcessAndRunToEntry(LPCSTR exeFileName,
                               LPSTR commandLine,
                               LPCSTR directory,
                               PROCESS_INFORMATION& processInfo,
                               bool console);

bool InjectDll(DWORD processId, const char* dllDir, const char* dllFileName);
