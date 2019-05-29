#pragma once
#include <Windows.h>

struct ExeInfo {
	size_t			entryPoint;
	bool            managed;
	bool            i386;
};

bool GetExeInfo(LPCSTR fileName, ExeInfo&info);