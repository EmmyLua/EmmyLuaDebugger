#include "emmy_hook.h"
#include <cassert>
#include <mutex>
#include <set>
#include <unordered_map>
#include <ShlObj.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include "libpe/libpe.h"
#include "../emmy_facade.h"

typedef int(*_lua_pcall)(lua_State* L, int nargs, int nresults, int errfunc);
typedef int(*_lua_pcallk)(lua_State* L, int nargs, int nresults, int errfunc, lua_KContext ctx, lua_KFunction k);

typedef HMODULE (WINAPI *LoadLibraryExW_t)(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags);
LoadLibraryExW_t LoadLibraryExW_dll = nullptr;

std::mutex mutexPostLoadModule;
std::set<std::string> loadedModules;

HOOK_STATUS Hook(void* InEntryPoint,
                 void* InHookProc,
                 void* InCallback,
                 HOOK_HANDLE OutHandle) {
	const auto hHook = new HOOK_TRACE_INFO();
	ULONG ACLEntries[1] = { 0 };
	HOOK_STATUS status = LhInstallHook(
		InEntryPoint,
		InHookProc,
		InCallback,
		hHook);
	assert(status == 0);
	status = LhSetExclusiveACL(ACLEntries, 0, hHook);
	assert(status == 0);
	return status;
}

HOOK_STATUS UnHook(HOOK_HANDLE InHandle) {
	ULONG ACLEntries[1] = {0};
	const HOOK_STATUS status = LhSetExclusiveACL(ACLEntries, 0, InHandle);
	return status;
}

int lua_pcall_worker(lua_State* L, int nargs, int nresults, int errfunc) {
	LPVOID lp;
	LhBarrierGetCallback(&lp);
	const auto pcall = (_lua_pcall)lp;
	EmmyFacade::Get()->Attach(L);
	return pcall(L, nargs, nresults, errfunc);
}

int lua_pcallk_worker(lua_State* L, int nargs, int nresults, int errfunc, lua_KContext ctx, lua_KFunction k) {
	LPVOID lp;
	LhBarrierGetCallback(&lp);
	const auto pcallk = (_lua_pcallk)lp;
	EmmyFacade::Get()->Attach(L);
	return pcallk(L, nargs, nresults, errfunc, ctx, k);
}

int lua_error_worker(lua_State *L) {
	typedef int (*dll_lua_error)(lua_State *);
	EmmyFacade::Get()->Attach(L);
	LPVOID lp;
	LhBarrierGetCallback(&lp);
	const auto error = (dll_lua_error)lp;
	EmmyFacade::Get()->BreakHere(L);
	return error(L);
}

#define HOOK(FN, WORKER, REQUIRED) {\
	const auto it = symbols.find(""#FN"");\
	if (it != symbols.end()) {\
		const auto ptr = (void*)it->second; \
		const auto hHook = new HOOK_TRACE_INFO(); \
		Hook(ptr, (void*)(WORKER), ptr, hHook); \
	} else if (REQUIRED) {\
		printf("[ERR]function %s not found.\n", ""#FN"");\
		return;\
	}\
}

void HookLuaFunctions(std::unordered_map<std::string, DWORD64>& symbols) {
	if (symbols.empty())
		return;
	// lua 5.1
	HOOK(lua_pcall, lua_pcall_worker, false);
	// lua 5.2
	HOOK(lua_pcallk, lua_pcallk_worker, false);
	HOOK(lua_error, lua_error_worker, true);
}

void LoadSymbolsRecursively(HANDLE hProcess, HMODULE hModule) {
	char moduleName[_MAX_PATH];
	ZeroMemory(moduleName, _MAX_PATH);
	DWORD nameLen = GetModuleBaseName(hProcess, hModule, moduleName, _MAX_PATH);
	if (nameLen == 0 || loadedModules.find(moduleName) != loadedModules.end())
		return;

	loadedModules.insert(moduleName);
	char modulePath[_MAX_PATH];
	// skip modules in c://WINDOWS
	{
		ZeroMemory(modulePath, _MAX_PATH);
		DWORD fileNameLen = GetModuleFileNameEx(hProcess, hModule, modulePath, _MAX_PATH);
		if (fileNameLen == 0)
			return;

		char windowsPath[MAX_PATH];
		if (SHGetFolderPath(nullptr, CSIDL_WINDOWS, nullptr, SHGFP_TYPE_CURRENT, windowsPath) == 0) {
			std::string module_path = modulePath;
			if (module_path.find(windowsPath) != std::string::npos) {
				return;
			}
		}
	}
	// skip emmy modules
	{
		static const char* emmyModules[] = {"emmy_hook.dll", "EasyHook.dll"};
		std::string module_path = modulePath;
		for (const char* emmyModuleName : emmyModules) {
			if (strcmp(moduleName, emmyModuleName) == 0)
				return;
		}
	}

	EmmyFacade::Get()->SendLog(LogType::Info, "analyze: %s", moduleName);

	PE pe = {};
	PE_STATUS st = peOpenFile(&pe, modulePath);
	std::unordered_map<std::string, DWORD64> symbols;

	if (st == PE_SUCCESS)
		st = peParseExportTable(&pe, INT32_MAX);
	if (st == PE_SUCCESS && PE_HAS_TABLE(&pe, ExportTable)) {
		PE_FOREACH_EXPORTED_SYMBOL(&pe, pSymbol) {
			if (PE_SYMBOL_HAS_NAME(pSymbol)) {
				const char* name = pSymbol->Name;
				if (name[0] == 'l' && name[1] == 'u' && name[2] == 'a') {
					auto addr = (uint64_t)hModule;
					addr += pSymbol->Address.VA - pe.qwBaseAddress;
					symbols[pSymbol->Name] = addr;

					EmmyFacade::Get()->SendLog(LogType::Info, "\t[B]Lua symbol: %s", name);
				}
			}
		}
	}

	HookLuaFunctions(symbols);

	// imports
	if (st == PE_SUCCESS)
		st = peParseImportTable(&pe);
	if (st == PE_SUCCESS && PE_HAS_TABLE(&pe, ImportTable))
	{
		PE_FOREACH_IMPORTED_MODULE(&pe, pModule)
		{
			HMODULE hImportModule = GetModuleHandle(pModule->Name);
			LoadSymbolsRecursively(hProcess, hImportModule);
		}
	}
}

void PostLoadLibrary(HMODULE hModule) {
	extern HINSTANCE g_hInstance;
	if (hModule == g_hInstance) {
		return;
	}

	HANDLE hProcess = GetCurrentProcess();

	char moduleName[_MAX_PATH];
	GetModuleBaseName(hProcess, hModule, moduleName, _MAX_PATH);

	std::lock_guard<std::mutex> lock(mutexPostLoadModule);
	LoadSymbolsRecursively(hProcess, hModule);
}

HMODULE WINAPI LoadLibraryExW_intercept(LPCWSTR fileName, HANDLE hFile, DWORD dwFlags) {
	// We have to call the loader lock (if it is available) so that we don't get deadlocks
	// in the case where Dll initialization acquires the loader lock and calls LoadLibrary
	// while another thread is inside PostLoadLibrary.
	HMODULE hModule = LoadLibraryExW_dll(fileName, hFile, dwFlags);

	if (hModule != nullptr) {
		PostLoadLibrary(hModule);
	}
	return hModule;
}

void HookLoadLibrary() {
	HMODULE hModuleKernel = GetModuleHandle("KernelBase.dll");
	if (hModuleKernel == nullptr)
		hModuleKernel = GetModuleHandle("kernel32.dll");
	if (hModuleKernel != nullptr) {
		// LoadLibraryExW is called by the other LoadLibrary functions, so we
		// only need to hook it.

		// TODO hook!!!
		LoadLibraryExW_dll = (LoadLibraryExW_t)GetProcAddress(hModuleKernel, "LoadLibraryExW");

		// destroy these functions.
		const auto hHook = new HOOK_TRACE_INFO();
		ULONG ACLEntries[1] = {0};
		HOOK_STATUS status = LhInstallHook(
			(void*)LoadLibraryExW_dll,
			(void*)LoadLibraryExW_intercept,
			(PVOID)nullptr,
			hHook);
		assert(status == 0);
		status = LhSetExclusiveACL(ACLEntries, 0, hHook);
		assert(status == 0);
	}
}

int StartupHookMode() {
	const int pid = (int)GetCurrentProcessId();
	EmmyFacade::Get()->StartupHookMode(pid);
	return 0;
}

void FindAndHook() {
	HookLoadLibrary();

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	if (hSnapshot) {
		MODULEENTRY32 module;
		module.dwSize = sizeof(MODULEENTRY32);
		BOOL moreModules = Module32First(hSnapshot, &module);
		while (moreModules) {
			PostLoadLibrary(module.hModule);
			moreModules = Module32Next(hSnapshot, &module);
		}
	}
	else {
		HMODULE module = GetModuleHandle(nullptr);
		PostLoadLibrary(module);
	}
	CloseHandle(hSnapshot);
}