#include "shared/shme.h"

bool CreateMemFile(SharedFile* file) {
	// Get a handle to our file map
	const auto MapFile = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, SHMEMSIZE, SHMEMNAME);
	if (MapFile == nullptr) {
		MessageBoxA(nullptr, "Failed to create file mapping!", "DLL_PROCESS_ATTACH", MB_OK | MB_ICONERROR);
		return false;
	}

	// Get our shared memory pointer
	const auto MemFile = MapViewOfFile(MapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (MemFile == nullptr) {
		MessageBoxA(nullptr, "Failed to map shared memory!", "DLL_PROCESS_ATTACH", MB_OK | MB_ICONERROR);
		return false;
	}
	file->lpMemFile = MemFile;
	file->hMapFile = MapFile;
	return true;
}

bool CloseMemFile(SharedFile* file) {
	UnmapViewOfFile(file->lpMemFile);
	CloseHandle(file->hMapFile);
	return true;
}

bool ReadSharedData(TSharedData& data) {
	SharedFile file = {};
	if (!CreateMemFile(&file)) {
		return false;
	}
	memcpy(&data, file.lpMemFile, SHMEMSIZE);
	// Clean up
	CloseMemFile(&file);
	return true;
}

bool WriteSharedData(HANDLE hMapFile, LPVOID lpMemFile, TSharedData& data) {
	memset(lpMemFile, 0, SHMEMSIZE);
	memcpy(lpMemFile, &data, sizeof(TSharedData));
	return true;
}