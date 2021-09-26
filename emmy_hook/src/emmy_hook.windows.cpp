#include "emmy_hook/emmy_hook.h"
#include <cassert>
#include <mutex>
#include <set>
#include <unordered_map>
#include "emmy_debugger/emmy_facade.h"
#include "emmy_debugger/api/lua_api.h"
#include <ShlObj.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include "easyhook.h"
#include "libpe/libpe.h"
#include "io.h"
#include "emmy_debugger/proto/socket_server_transporter.h"
#include "shared/shme.h"

typedef TRACED_HOOK_HANDLE HOOK_HANDLE;
typedef NTSTATUS HOOK_STATUS;

HOOK_STATUS Hook(void* InEntryPoint,
                 void* InHookProc,
                 void* InCallback,
                 HOOK_HANDLE OutHandle);

HOOK_STATUS UnHook(HOOK_HANDLE InHandle);

typedef int (*_lua_pcall)(lua_State* L, int nargs, int nresults, int errfunc);

typedef int (*_lua_pcallk)(lua_State* L, int nargs, int nresults, int errfunc, lua_KContext ctx, lua_KFunction k);

typedef int (*_lua_resume)(lua_State* L, lua_State* from, int nargs, int* nresults);

typedef HMODULE (WINAPI *LoadLibraryExW_t)(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags);

LoadLibraryExW_t LoadLibraryExW_dll = nullptr;

std::mutex mutexPostLoadModule;
std::set<std::string> loadedModules;

HOOK_STATUS Hook(void* InEntryPoint,
                 void* InHookProc,
                 void* InCallback,
                 HOOK_HANDLE OutHandle)
{
	const auto hHook = new HOOK_TRACE_INFO();
	ULONG ACLEntries[1] = {0};
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

HOOK_STATUS UnHook(HOOK_HANDLE InHandle)
{
	ULONG ACLEntries[1] = {0};
	const HOOK_STATUS status = LhSetExclusiveACL(ACLEntries, 0, InHandle);
	return status;
}

int lua_pcall_worker(lua_State* L, int nargs, int nresults, int errfunc)
{
	LPVOID lp;
	LhBarrierGetCallback(&lp);
	const auto pcall = (_lua_pcall)lp;
	EmmyFacade::Get().Attach(L);
	return pcall(L, nargs, nresults, errfunc);
}

int lua_pcallk_worker(lua_State* L, int nargs, int nresults, int errfunc, lua_KContext ctx, lua_KFunction k)
{
	LPVOID lp;
	LhBarrierGetCallback(&lp);
	const auto pcallk = (_lua_pcallk)lp;
	EmmyFacade::Get().Attach(L);
	return pcallk(L, nargs, nresults, errfunc, ctx, k);
}

int lua_error_worker(lua_State* L)
{
	typedef int (*dll_lua_error)(lua_State*);
	EmmyFacade::Get().Attach(L);
	LPVOID lp;
	LhBarrierGetCallback(&lp);
	const auto error = (dll_lua_error)lp;
	// EmmyFacade::Get().BreakHere(L);
	return error(L);
}

int lua_resume_worker(lua_State* L, lua_State* from, int nargs, int* nresults)
{
	LPVOID lp;
	LhBarrierGetCallback(&lp);
	const auto luaResume = (_lua_resume)lp;
	EmmyFacade::Get().Attach(L);
	return luaResume(L, from, nargs, nresults);
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

void HookLuaFunctions(std::unordered_map<std::string, DWORD64>& symbols)
{
	if (symbols.empty())
		return;
	// lua 5.1
	HOOK(lua_pcall, lua_pcall_worker, false);
	// lua 5.2
	HOOK(lua_pcallk, lua_pcallk_worker, false);
	// HOOK(lua_error, lua_error_worker, true);
	HOOK(lua_resume, lua_resume_worker, false);
}

void LoadSymbolsRecursively(HANDLE hProcess, HMODULE hModule)
{
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
		if (SHGetFolderPath(nullptr, CSIDL_WINDOWS, nullptr, SHGFP_TYPE_CURRENT, windowsPath) == 0)
		{
			std::string module_path = modulePath;
			if (module_path.find(windowsPath) != std::string::npos)
			{
				return;
			}
		}
	}
	// skip emmy modules
	{
		static const char* emmyModules[] = {"emmy_hook.dll", "EasyHook.dll"};
		std::string module_path = modulePath;
		for (const char* emmyModuleName : emmyModules)
		{
			if (strcmp(moduleName, emmyModuleName) == 0)
				return;
		}
	}

	EmmyFacade::Get().SendLog(LogType::Info, "analyze: %s", moduleName);

	PE pe = {};
	PE_STATUS st = peOpenFile(&pe, modulePath);
	std::unordered_map<std::string, DWORD64> symbols;

	if (st == PE_SUCCESS)
		st = peParseExportTable(&pe, INT32_MAX);
	if (st == PE_SUCCESS && PE_HAS_TABLE(&pe, ExportTable))
	{
		PE_FOREACH_EXPORTED_SYMBOL(&pe, pSymbol)
		{
			if (PE_SYMBOL_HAS_NAME(pSymbol))
			{
				const char* name = pSymbol->Name;
				if (name[0] == 'l' && name[1] == 'u' && name[2] == 'a')
				{
					auto addr = (uint64_t)hModule;
					addr += pSymbol->Address.VA - pe.qwBaseAddress;
					symbols[pSymbol->Name] = addr;

					EmmyFacade::Get().SendLog(LogType::Info, "\t[B]Lua symbol: %s", name);
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

void PostLoadLibrary(HMODULE hModule)
{
	extern HINSTANCE g_hInstance;
	if (hModule == g_hInstance)
	{
		return;
	}

	HANDLE hProcess = GetCurrentProcess();

	char moduleName[_MAX_PATH];
	GetModuleBaseName(hProcess, hModule, moduleName, _MAX_PATH);

	std::lock_guard<std::mutex> lock(mutexPostLoadModule);
	LoadSymbolsRecursively(hProcess, hModule);
}

HMODULE WINAPI LoadLibraryExW_intercept(LPCWSTR fileName, HANDLE hFile, DWORD dwFlags)
{
	// We have to call the loader lock (if it is available) so that we don't get deadlocks
	// in the case where Dll initialization acquires the loader lock and calls LoadLibrary
	// while another thread is inside PostLoadLibrary.
	HMODULE hModule = LoadLibraryExW_dll(fileName, hFile, dwFlags);

	if (hModule != nullptr)
	{
		PostLoadLibrary(hModule);
	}
	return hModule;
}

void HookLoadLibrary()
{
	HMODULE hModuleKernel = GetModuleHandle("KernelBase.dll");
	if (hModuleKernel == nullptr)
		hModuleKernel = GetModuleHandle("kernel32.dll");
	if (hModuleKernel != nullptr)
	{
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

void redirect(int port)
{
	HANDLE readStdPipe = NULL, writeStdPipe = NULL;

	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = nullptr;

	CreatePipe(&readStdPipe, &writeStdPipe, &saAttr, 0);

	const auto oldStdout = _dup(_fileno(stdout));
	const auto oldStderr = _dup(_fileno(stderr));

	if (oldStdout == -1 || oldStderr == -1)
	{
		printf("stdout or stderr redirect error");
		if (oldStdout != -1)
		{
			_close(oldStdout);
		}
		if (oldStderr != -1)
		{
			_close(oldStderr);
		}

		return;
	}

	const auto stream = _open_osfhandle(reinterpret_cast<long>(writeStdPipe), 0);
	FILE* capture = nullptr;
	if (stream == -1)
	{
		printf("capture stream open fail");
		_dup2(oldStdout, _fileno(stdout));
		_dup2(oldStderr, _fileno(stderr));

		_close(oldStdout);
		_close(oldStderr);

		return;
	}
	capture = _fdopen(stream, "wt");

	// stdout now refers to file "capture" 
	if (_dup2(_fileno(capture), _fileno(stdout)) == -1)
	{
		printf("Can't _dup2 stdout to capture");

		_dup2(oldStdout, _fileno(stdout));
		_dup2(oldStderr, _fileno(stderr));

		_close(oldStdout);
		_close(oldStderr);

		return;
	}

	// stderr now refers to file "capture" 
	if (_dup2(_fileno(capture), _fileno(stderr)) == -1)
	{
		printf("Can't _dup2 stderr to capture");

		_dup2(oldStdout, _fileno(stdout));
		_dup2(oldStderr, _fileno(stderr));

		_close(oldStdout);
		_close(oldStderr);

		return;
	}
	setvbuf(stdout, nullptr, _IONBF, 0);
	setvbuf(stderr, nullptr, _IONBF, 0);
	// 1024 - 65535
	while (port > 0xffff) port -= 0xffff;
	while (port < 0x400) port += 0x400;
	port++;

	const auto transport = std::make_shared<SocketServerTransporter>();
	std::string err;
	const auto suc = transport->Listen("localhost", port, err);

	if (!suc)
	{
		printf("capture log error : %s", err.c_str());

		_dup2(oldStdout, _fileno(stdout));
		_dup2(oldStderr, _fileno(stderr));

		_close(oldStdout);
		_close(oldStderr);

		return;
	}

	std::thread thread([transport,readStdPipe,oldStdout]()
	{
		char buf[1024] = {0};
		while (true)
		{
			DWORD readWord;
			ZeroMemory(buf, 1024);
			const bool suc = ReadFile(readStdPipe, buf, 1024, &readWord, nullptr);

			if (suc && readWord > 0)
			{
				_write(oldStdout, buf, readWord);
				transport->Send(buf, readWord);
			}
		}
	});
	thread.detach();
}

int StartupHookMode(void* lpParam)
{
	const int pid = (int)GetCurrentProcessId();
	EmmyFacade::Get().StartupHookMode(pid);

	if (lpParam != nullptr && ((RemoteThreadParam*)lpParam)->bRedirect)
	{
		redirect(pid);
	}

	return 0;
}

void FindAndHook()
{
	HookLoadLibrary();

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	if (hSnapshot)
	{
		MODULEENTRY32 module;
		module.dwSize = sizeof(MODULEENTRY32);
		BOOL moreModules = Module32First(hSnapshot, &module);
		while (moreModules)
		{
			PostLoadLibrary(module.hModule);
			moreModules = Module32Next(hSnapshot, &module);
		}
	}
	else
	{
		HMODULE module = GetModuleHandle(nullptr);
		PostLoadLibrary(module);
	}
	CloseHandle(hSnapshot);
}
