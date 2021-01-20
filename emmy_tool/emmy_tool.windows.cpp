#include "emmy_tool.h"
#include <Windows.h>
#include <ImageHlp.h>
#include <string>
#include <iostream>
#include "utility.h"
#include "shme.h"

void SetBreakpoint(HANDLE hProcess, LPVOID entryPoint, bool set, BYTE* data);

bool StartProcessAndRunToEntry(LPCSTR exeFileName,
                               LPSTR commandLine,
                               LPCSTR directory,
                               PROCESS_INFORMATION& processInfo,
                               bool console) {
	STARTUPINFO startUpInfo{};
	startUpInfo.cb = sizeof(startUpInfo);

	ExeInfo info{};
	if (!GetExeInfo(exeFileName, info) || info.entryPoint == 0) {
		//MessageEvent("Error: The entry point for the application could not be located", MessageType_Error);
		return false;
	}

	DWORD flags = DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS;
	if (console)
		flags |= CREATE_NEW_CONSOLE;
	else
		flags |= CREATE_NO_WINDOW;

	if (!CreateProcess(nullptr,
	                   commandLine,
	                   nullptr,
	                   nullptr,
	                   TRUE,
	                   flags,
	                   nullptr,
	                   directory,
	                   &startUpInfo,
	                   &processInfo)) {
		//OutputError(GetLastError());
		return false;
	}

	// Running to the entry point currently doesn't work for managed applications, so
	// just start it up.

	if (!info.managed) {

		size_t entryPoint = info.entryPoint;

		BYTE breakPointData;
		bool done = false;

		while (!done) {

			DEBUG_EVENT debugEvent;
			WaitForDebugEvent(&debugEvent, INFINITE);

			DWORD continueStatus = DBG_EXCEPTION_NOT_HANDLED;

			if (debugEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
				if (debugEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP ||
					debugEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT) {

					CONTEXT context;
					context.ContextFlags = CONTEXT_FULL;

					GetThreadContext(processInfo.hThread, &context);
#if _WIN64
					if (context.Rip == entryPoint + 1)
#else
					if (context.Eip == entryPoint + 1)
#endif
					{

						// Restore the original code bytes.
						SetBreakpoint(processInfo.hProcess, (LPVOID)entryPoint, false, &breakPointData);
						done = true;

						// Backup the instruction pointer so that we execute the original instruction.
#if _WIN64
						--context.Rip;
#else
						--context.Eip;
#endif
						SetThreadContext(processInfo.hThread, &context);

						// Suspend the thread before we continue the debug event so that the program
						// doesn't continue to run.
						SuspendThread(processInfo.hThread);
						
					}

					continueStatus = DBG_CONTINUE;
				}
			}
			else if (debugEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
				done = true;
			}
			else if (debugEvent.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
				// Offset the entry point by the load address of the process.
				entryPoint += reinterpret_cast<size_t>(debugEvent.u.CreateProcessInfo.lpBaseOfImage);

				// Write a break point at the entry point of the application so that we
				// will stop when we reach that point.
				SetBreakpoint(processInfo.hProcess, reinterpret_cast<void*>(entryPoint), true, &breakPointData);

				CloseHandle(debugEvent.u.CreateProcessInfo.hFile);

			}
			else if (debugEvent.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
				CloseHandle(debugEvent.u.LoadDll.hFile);
			}

			ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, continueStatus);
		}
	}

	DebugActiveProcessStop(processInfo.dwProcessId);
	return true;
}

void SetBreakpoint(HANDLE hProcess, LPVOID entryPoint, bool set, BYTE* data) {

	DWORD protection;

	// Give ourself write access to the region.
	if (VirtualProtectEx(hProcess, entryPoint, 1, PAGE_EXECUTE_READWRITE, &protection)) {

		BYTE buffer[1];

		if (set) {
			SIZE_T numBytesRead;
			ReadProcessMemory(hProcess, entryPoint, data, 1, &numBytesRead);

			// Write the int 3 instruction.
			buffer[0] = 0xCC;
		}
		else {
			// Restore the original byte.
			buffer[0] = data[0];
		}

		SIZE_T numBytesWritten;
		WriteProcessMemory(hProcess, entryPoint, buffer, 1, &numBytesWritten);

		// Restore the original protections.
		VirtualProtectEx(hProcess, entryPoint, 1, protection, &protection);

		// Flush the cache so we know that our new code gets executed.
		FlushInstructionCache(hProcess, entryPoint, 1);
	}
}

enum MessageType {
	MessageType_Info = 0,
	MessageType_Warning = 1,
	MessageType_Error = 2
};

void* RemoteDup(HANDLE process, const void* source, size_t length) {
	void* remote = VirtualAllocEx(process, nullptr, length, MEM_COMMIT, PAGE_READWRITE);
	SIZE_T numBytesWritten;
	WriteProcessMemory(process, remote, source, length, &numBytesWritten);
	return remote;
}

void MessageEvent(const std::string& message, MessageType type = MessageType_Info) {
	if (type == MessageType_Error)
		std::cerr << "[F]" << message << std::endl;
	else
		std::cout << "[F]" << message << std::endl;
}

void OutputError(DWORD error) {
	char buffer[1024];
	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error, 0, buffer, 1024, nullptr)) {
		std::string message = "Error: ";
		message += buffer;
		MessageEvent(message, MessageType_Error);
	}
}

bool GetStartupDirectory(char* path, int maxPathLength) {
	if (!GetModuleFileName(nullptr, path, maxPathLength)) {
		return false;
	}

	char* lastSlash = strrchr(path, '\\');

	if (lastSlash == nullptr) {
		return false;
	}

	// Terminate the path after the last slash.

	lastSlash[1] = 0;
	return true;
}

bool ExecuteRemoteKernelFuntion(HANDLE process, const char* functionName, LPVOID param, DWORD& exitCode) {
	HMODULE kernelModule = GetModuleHandle("Kernel32");
	FARPROC function = GetProcAddress(kernelModule, functionName);

	if (function == nullptr) {
		return false;
	}

	DWORD threadId;
	HANDLE thread = CreateRemoteThread(process,
		nullptr,
		0,
		(LPTHREAD_START_ROUTINE)function,
		param,
		0,
		&threadId);

	if (thread != nullptr) {
		WaitForSingleObject(thread, INFINITE);
		GetExitCodeThread(thread, &exitCode);

		CloseHandle(thread);
		return true;
	}
	return false;
}

bool IsBeingInjected(DWORD processId, LPCSTR moduleFileName) {
	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

	if (process == nullptr) {
		return false;
	}

	bool result = false;

	DWORD exitCode;
	void* remoteFileName = RemoteDup(process, moduleFileName, strlen(moduleFileName) + 1);

	if (ExecuteRemoteKernelFuntion(process, "GetModuleHandleA", remoteFileName, exitCode)) {
		result = exitCode != 0;
	}

	if (remoteFileName != nullptr) {
		VirtualFreeEx(process, remoteFileName, 0, MEM_RELEASE);
	}

	if (process != nullptr) {
		CloseHandle(process);
	}
	return result;
}

bool InjectDll(DWORD processId, const char* dllDir, const char* dllFileName) {
	if (IsBeingInjected(processId, dllFileName)) {
		MessageEvent("The process already attached.");
		return true;
	}

	MessageEvent("Start inject dll ...");
	bool success = true;

	const HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

	if (process == nullptr) {
		MessageEvent("Failed to open process", MessageType_Error);
		OutputError(GetLastError());
		return false;
	}
	DWORD exitCode = 0;

	// Set dll directory
	void* dllDirRemote = RemoteDup(process, dllDir, strlen(dllDir) + 1);
	ExecuteRemoteKernelFuntion(process, "SetDllDirectoryA", dllDirRemote, exitCode);
	VirtualFreeEx(process, dllDirRemote, 0, MEM_RELEASE);

	// Load the DLL.
	void* remoteFileName = RemoteDup(process, dllFileName, strlen(dllFileName) + 1);
	success &= ExecuteRemoteKernelFuntion(process, "LoadLibraryA", remoteFileName, exitCode);
	VirtualFreeEx(process, remoteFileName, 0, MEM_RELEASE);
	if (!success || exitCode == 0) {
		MessageEvent("Failed to load library", MessageType_Error);
		return false;
	}

	// Read shared data & call 'StartupHookMode()'
	TSharedData data;
	if (ReadSharedData(data)) {
		DWORD threadId;
		HANDLE thread = CreateRemoteThread(process,
			nullptr,
			0,
			(LPTHREAD_START_ROUTINE)data.lpInit,
			nullptr,
			0,
			&threadId);

		if (thread != nullptr) {
			WaitForSingleObject(thread, INFINITE);
			GetExitCodeThread(thread, &exitCode);

			CloseHandle(thread);
			success = true;
		}
		else {
			success = false;
		}
	}

	// Reset dll directory
	ExecuteRemoteKernelFuntion(process, "SetDllDirectoryA", nullptr, exitCode);

	if (process != nullptr) {
		CloseHandle(process);
	}

	if (success) {
		MessageEvent("Successfully inject dll!");
	}
	return success;
}