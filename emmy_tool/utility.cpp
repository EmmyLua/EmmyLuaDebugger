#include "utility.h"
#include <ImageHlp.h>

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
