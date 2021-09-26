/*
* Copyright (c) 2019. tangzx(love.tangzx@qq.com)
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "emmy_debugger/emmy_facade.h"
#include "emmy_hook/emmy_hook.h"
#if WIN32
#include <Windows.h>
#include "shared/shme.h"
static SharedFile file;

HINSTANCE g_hInstance = NULL;

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD reason, LPVOID reserved) {
	g_hInstance = hModule;
	EmmyFacade::Get().SetWorkMode(WorkMode::Attach);
	EmmyFacade::Get().StartHook = FindAndHook;
	
	if (reason == DLL_PROCESS_ATTACH) {
		TSharedData data;
		DisableThreadLibraryCalls(hModule);
		if (!CreateMemFile(&file)) {
			return FALSE;
		}
		// Set shared memory to hold what our remote process needs
		memset(file.lpMemFile, 0, SHMEMSIZE);
		data.hModule = hModule;
		data.lpInit = LPDWORD(StartupHookMode);
		data.dwOffset = DWORD(data.lpInit) - DWORD(data.hModule);
		memcpy(file.lpMemFile, &data, sizeof(TSharedData));
	}
	else if (reason == DLL_PROCESS_DETACH) {
		// Destroy();
		CloseMemFile(&file);
	}

	return TRUE;
}
#endif

