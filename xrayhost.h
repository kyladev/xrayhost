#pragma once
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllimport) BOOL     __stdcall XH_Init(LPCWSTR dllPathOrNull);
__declspec(dllimport) int      __stdcall XH_StartJson(const char* json);
__declspec(dllimport) int      __stdcall XH_ReloadJson(const char* json);
__declspec(dllimport) int      __stdcall XH_StartFile(LPCWSTR jsonPath);
__declspec(dllimport) int      __stdcall XH_ReloadFile(LPCWSTR jsonPath);
__declspec(dllimport) int      __stdcall XH_Stop(void);
__declspec(dllimport) unsigned __stdcall XH_LastErrorA(char* buf, unsigned cap);
__declspec(dllimport) unsigned __stdcall XH_PollLogA(char* buf, unsigned cap);
__declspec(dllimport) unsigned __stdcall XH_VersionA(char* buf, unsigned cap);
#ifdef __cplusplus
}
#endif
