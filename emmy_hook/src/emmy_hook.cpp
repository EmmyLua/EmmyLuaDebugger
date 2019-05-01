#include "emmy_hook.h"

HOOK_STATUS Hook(void* InEntryPoint,
	void* InHookProc,
	void* InCallback,
	HOOK_HANDLE OutHandle)
{
	auto hHook = new HOOK_HANDLE();
	const HOOK_STATUS status = LhInstallHook(
		InEntryPoint,
		InHookProc,
		InCallback,
		*hHook);
	if (status != 0)
		hHook = nullptr;
	return status;
}

HOOK_STATUS UnHook(HOOK_HANDLE InHandle)
{
	ULONG ACLEntries[1] = { 0 };
	const HOOK_STATUS status = LhSetExclusiveACL(ACLEntries, 0, InHandle);
	return status;
}
