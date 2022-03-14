#include "utility.h"
#include <ImageHlp.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <ShlObj.h>
#include <string>
#include <vector>

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
