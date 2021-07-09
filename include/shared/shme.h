#ifndef _SHME_H_
#define _SHME_H_

#include <Windows.h>

// Data struct to be shared between processes
struct TSharedData {
	DWORD dwOffset = 0;
	HMODULE hModule = nullptr;
	LPDWORD lpInit = nullptr;
};

struct SharedFile {
	HANDLE hMapFile;
	LPVOID lpMemFile;
};

struct RemoteThreadParam
{
	BOOL bRedirect;
};

// Size (in bytes) of data to be shared
#define SHMEMSIZE sizeof(TSharedData)
// Name of the shared file map (NOTE: Global namespaces must have the SeCreateGlobalPrivilege privilege)
#define SHMEMNAME "InjectedDllName_SHMEM"

bool CreateMemFile(SharedFile* file);

bool CloseMemFile(SharedFile* file);

bool ReadSharedData(TSharedData& data);

bool WriteSharedData(HANDLE hMapFile, LPVOID lpMemFile, TSharedData& data);
#endif