#pragma once
#include "easyhook.h"

typedef TRACED_HOOK_HANDLE HOOK_HANDLE;
typedef NTSTATUS HOOK_STATUS;

HOOK_STATUS Hook(void* InEntryPoint,
	void* InHookProc,
	void* InCallback,
	HOOK_HANDLE OutHandle);

HOOK_STATUS UnHook(HOOK_HANDLE InHandle);