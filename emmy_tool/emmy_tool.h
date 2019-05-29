#pragma once
#include <Windows.h>

bool InjectDll(DWORD processId, const char* dllDir, const char* dllFileName);