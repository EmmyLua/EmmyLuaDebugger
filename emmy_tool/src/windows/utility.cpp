#include "utility.h"
#include <ImageHlp.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <ShlObj.h>
#include <string>
#include <vector>
#include <thread>

bool GetExeInfo(LPCSTR fileName, ExeInfo& info) {
	LOADED_IMAGE loadedImage;
	if (!MapAndLoad(const_cast<PSTR>(fileName), nullptr, &loadedImage, FALSE, TRUE)) {
		return false;
	}

	info.managed = false;
	if (loadedImage.FileHeader->Signature == IMAGE_NT_SIGNATURE) {
		const DWORD netHeaderAddress =
			loadedImage.FileHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress;

		if (netHeaderAddress) {
			info.managed = true;
		}
	}

	info.entryPoint = loadedImage.FileHeader->OptionalHeader.AddressOfEntryPoint;
	info.i386 = loadedImage.FileHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386;

	UnMapAndLoad(&loadedImage);

	return true;
}

int GetProcessByName(const char* name) {
	DWORD pid = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 process;
	ZeroMemory(&process, sizeof(process));
	process.dwSize = sizeof(process);
	if (Process32First(snapshot, &process)) {
		do {
			if (strcmp(process.szExeFile, name) == 0) {
				pid = process.th32ProcessID;
				break;
			}
		}
		while (Process32Next(snapshot, &process));
	}
	CloseHandle(snapshot);
	return pid;
}


HWND GetProcessWindow(DWORD processId) {
	HWND hWnd = FindWindowEx(nullptr, nullptr, nullptr, nullptr);

	while (hWnd != nullptr) {
		if (GetParent(hWnd) == nullptr && GetWindowTextLength(hWnd) > 0 && IsWindowVisible(hWnd)) {
			DWORD windowProcessId;
			GetWindowThreadProcessId(hWnd, &windowProcessId);

			if (windowProcessId == processId) {
				// Found a match.
				break;
			}
		}
		hWnd = GetWindow(hWnd, GW_HWNDNEXT);
	}
	return hWnd;
}

void GetProcesses(std::vector<Process>& processes) {
	static char fileName[_MAX_PATH];
	// Get the id of this process so that we can filter it out of the list.
	DWORD currentProcessId = GetCurrentProcessId();

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (snapshot != INVALID_HANDLE_VALUE) {

		PROCESSENTRY32 processEntry = {0};
		processEntry.dwSize = sizeof(processEntry);

		if (Process32First(snapshot, &processEntry)) {
			do {
				if (processEntry.th32ProcessID != currentProcessId && processEntry.th32ProcessID != 0) {

					Process process;

					process.id = processEntry.th32ProcessID;
					process.name = processEntry.szExeFile;

					HANDLE m_process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processEntry.th32ProcessID);
					if (m_process) {
						int err = GetModuleFileNameEx(m_process, nullptr, fileName, _MAX_PATH);
						if (err == 0) // ERROR_ACCESS_DENIED = 5
							process.path = "error";
						else process.path = fileName;

						if (!process.path.empty()) {
							char windowsPath[MAX_PATH];
							if (SHGetFolderPath(nullptr, CSIDL_WINDOWS, nullptr, SHGFP_TYPE_CURRENT, windowsPath) == 0) {
								if (process.path.find(windowsPath) == std::string::npos) {

									HWND hWnd = GetProcessWindow(processEntry.th32ProcessID);

									if (hWnd != nullptr) {
										char buffer[1024];
										GetWindowText(hWnd, buffer, 1024);
										process.title = buffer;
									}
									processes.push_back(process);
								}
							}
						}
					}
				}
			}
			while (Process32Next(snapshot, &processEntry));
		}
		CloseHandle(snapshot);
	}
}


void SetBreakpoint(HANDLE hProcess, LPVOID entryPoint, bool set, BYTE *data) {
	DWORD protection;

	// Give ourself write access to the region.
	if (VirtualProtectEx(hProcess, entryPoint, 1, PAGE_EXECUTE_READWRITE, &protection)) {
		BYTE buffer[1];

		if (set) {
			SIZE_T numBytesRead;
			ReadProcessMemory(hProcess, entryPoint, data, 1, &numBytesRead);

			// Write the int 3 instruction.
			buffer[0] = 0xCC;
		} else {
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

void MessageEvent(const std::string &message, MessageType type = MessageType_Info) {
	if (type == MessageType_Error) {
		std::cerr << "[F]" << message << std::endl;
	} else {
		std::cerr << "[F]" << message << std::endl;
	}
}

void *RemoteDup(HANDLE process, const void *source, size_t length) {
	void *remote = VirtualAllocEx(process, nullptr, length, MEM_COMMIT, PAGE_READWRITE);
	SIZE_T numBytesWritten;
	WriteProcessMemory(process, remote, source, length, &numBytesWritten);
	return remote;
}



void OutputError(DWORD error) {
	char buffer[1024];
	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error, 0, buffer, 1024, nullptr)) {
		std::string message = "Error: ";
		message += buffer;
		MessageEvent(message, MessageType_Error);
	}
}

bool GetStartupDirectory(char *path, int maxPathLength) {
	if (!GetModuleFileName(nullptr, path, maxPathLength)) {
		return false;
	}

	char *lastSlash = strrchr(path, '\\');

	if (lastSlash == nullptr) {
		return false;
	}

	// Terminate the path after the last slash.

	lastSlash[1] = 0;
	return true;
}

bool ExecuteRemoteKernelFuntion(HANDLE process, const char *functionName, LPVOID param, DWORD &exitCode) {
	HMODULE kernelModule = GetModuleHandle("Kernel32");
	FARPROC function = GetProcAddress(kernelModule, functionName);

	if (function == nullptr) {
		return false;
	}

	DWORD threadId;
	HANDLE thread = CreateRemoteThread(process,
	                                   nullptr,
	                                   0,
	                                   (LPTHREAD_START_ROUTINE) function,
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
	void *remoteFileName = RemoteDup(process, moduleFileName, strlen(moduleFileName) + 1);

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

bool InjectDll(DWORD processId, const char *dllDir, const char *dllFileName, bool capture) {
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
	void *dllDirRemote = RemoteDup(process, dllDir, strlen(dllDir) + 1);
	ExecuteRemoteKernelFuntion(process, "SetDllDirectoryA", dllDirRemote, exitCode);
	VirtualFreeEx(process, dllDirRemote, 0, MEM_RELEASE);

	// Load the DLL.
	void *remoteFileName = RemoteDup(process, dllFileName, strlen(dllFileName) + 1);
	success &= ExecuteRemoteKernelFuntion(process, "LoadLibraryA", remoteFileName, exitCode);
	VirtualFreeEx(process, remoteFileName, 0, MEM_RELEASE);
	if (!success || exitCode == 0) {
		MessageEvent("Failed to load library", MessageType_Error);
		return false;
	}


	void *lpParam = nullptr;
	if (capture) {
		lpParam = (void *) VirtualAllocEx(process, 0, sizeof(RemoteThreadParam), MEM_COMMIT,
		                                  PAGE_READWRITE);
		RemoteThreadParam param;
		param.bRedirect = TRUE;

		::WriteProcessMemory(process, lpParam, &param, sizeof(RemoteThreadParam), NULL);
	}

	// Read shared data & call 'StartupHookMode()'
	TSharedData data;
	if (ReadSharedData(data)) {
		DWORD threadId;
		HANDLE thread = CreateRemoteThread(process,
		                                   nullptr,
		                                   0,
		                                   (LPTHREAD_START_ROUTINE) data.lpInit,
		                                   (void *) lpParam,
		                                   0,
		                                   &threadId);

		if (thread != nullptr) {
			WaitForSingleObject(thread, INFINITE);
			GetExitCodeThread(thread, &exitCode);

			CloseHandle(thread);
			success = true;
		} else {
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

// 和InjectDll 实现不同的是，不会closeHandle
bool InjectDllForProcess(HANDLE hProcess, const char *dllDir, const char *dllFileName) {
	bool success = true;
	DWORD exitCode = 0;

	// Set dll directory
	void *dllDirRemote = RemoteDup(hProcess, dllDir, strlen(dllDir) + 1);
	ExecuteRemoteKernelFuntion(hProcess, "SetDllDirectoryA", dllDirRemote, exitCode);
	VirtualFreeEx(hProcess, dllDirRemote, 0, MEM_RELEASE);

	// Load the DLL.
	void *remoteFileName = RemoteDup(hProcess, dllFileName, strlen(dllFileName) + 1);
	success &= ExecuteRemoteKernelFuntion(hProcess, "LoadLibraryA", remoteFileName, exitCode);
	VirtualFreeEx(hProcess, remoteFileName, 0, MEM_RELEASE);
	if (!success || exitCode == 0) {
		MessageEvent("Failed to load library", MessageType_Error);
		return false;
	}

	// Read shared data & call 'StartupHookMode()'
	TSharedData data;
	if (ReadSharedData(data)) {
		DWORD threadId;
		HANDLE thread = CreateRemoteThread(hProcess,
		                                   nullptr,
		                                   0,
		                                   (LPTHREAD_START_ROUTINE) data.lpInit,
		                                   nullptr,
		                                   0,
		                                   &threadId);

		if (thread != nullptr) {
			WaitForSingleObject(thread, INFINITE);
			GetExitCodeThread(thread, &exitCode);
			CloseHandle(thread);
			success = true;
		} else {
			success = false;
		}
	}

	// Reset dll directory
	ExecuteRemoteKernelFuntion(hProcess, "SetDllDirectoryA", nullptr, exitCode);

	if (success) {
		MessageEvent("Successfully inject dll!");
	}
	return success;
}


void ReceiveLog(DWORD processId) {
	int debugPort = processId;
	// 1024 - 65535
	while (debugPort > 0xffff) debugPort -= 0xffff;
	while (debugPort < 0x400) debugPort += 0x400;
	debugPort++;

	// 开始连接 debug session
	// 开一个网络连接
	WORD version MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(version, &data);

	sockaddr_in serverAddress;
	SOCKET hSocket = NULL;

	hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (hSocket == INVALID_SOCKET) {
		return;
	}

	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons((short) debugPort);

	if (SOCKET_ERROR == connect(hSocket, (sockaddr *) &serverAddress, sizeof(serverAddress))) {
		closesocket(hSocket);
		DWORD error = WSAGetLastError();
		printf("tcp connect error, err code=%d", error);
		return;
	}

	char receiveBuf[10240] = {0};

	while (true) {
		int receiveSize = ::recv(hSocket, receiveBuf, sizeof(receiveBuf), 0);
		if (receiveSize <= 0) {
			break;
		}

		fwrite(receiveBuf, sizeof(char), receiveSize, stdout);
	}
}
