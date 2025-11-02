// Build (x64):
//   cl /nologo /LD /EHsc /std:c++17 XrayHost.cpp /link /MACHINE:X64
// Build (x86):
//   cl /nologo /LD /EHsc /std:c++17 XrayHost.cpp /link /MACHINE:X86
// Exports (C ABI, __stdcall):
//   BOOL  XH_Init(LPCWSTR dllPathOrNull)         // loads xraywrapper.dll (optional path; nullptr => "xraywrapper.dll")
//   int   XH_StartJson(const char* json)         // start from JSON string
//   int   XH_ReloadJson(const char* json)        // reload from JSON string
//   int   XH_StartFile(LPCWSTR jsonPath)         // start from JSON file (UTF-16 path)
//   int   XH_ReloadFile(LPCWSTR jsonPath)        // reload from JSON file (UTF-16 path)
//   int   XH_Stop()                              
//   UINT  XH_LastErrorA(char* buf, UINT cap)     // copy last error (ANSI/UTF-8)
//   UINT  XH_PollLogA(char* buf, UINT cap)       // copy any pending log bytes
//   UINT  XH_VersionA(char* buf, UINT cap)       // copy version string
//cl /nologo /LD /EHsc /std:c++17 XrayHost.cpp /link /MACHINE:X64
//keep xraywrapper.dll next to your xraywrapper.dll or pass a full path to XH_Init.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")

static HMODULE g_xray = nullptr;

using fn_Xray_Version   = const char* (__cdecl *)(void);
using fn_Xray_Start     = int         (__cdecl *)(const char*);
using fn_Xray_Reload    = int         (__cdecl *)(const char*);
using fn_Xray_Stop      = int         (__cdecl *)(void);
using fn_Xray_LastError = unsigned int(__cdecl *)(char*, unsigned int);
using fn_Xray_PollLog   = unsigned int(__cdecl *)(char*, unsigned int);

static fn_Xray_Version   pVersion   = nullptr;
static fn_Xray_Start     pStart     = nullptr;
static fn_Xray_Reload    pReload    = nullptr;
static fn_Xray_Stop      pStop      = nullptr;
static fn_Xray_LastError pLastError = nullptr;
static fn_Xray_PollLog   pPollLog   = nullptr;

static std::string ReadAllBytes(LPCWSTR path) {
    std::string out;
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return out;
    LARGE_INTEGER sz{};
    if (!GetFileSizeEx(h, &sz) || sz.QuadPart <= 0 || sz.QuadPart > 64ll*1024*1024) {
        CloseHandle(h);
        return out;
    }
    out.resize(static_cast<size_t>(sz.QuadPart));
    DWORD read = 0;
    if (!ReadFile(h, out.data(), static_cast<DWORD>(out.size()), &read, nullptr) || read != out.size()) {
        out.clear();
    }
    CloseHandle(h);
    return out;
}

static bool EnsureLoaded(LPCWSTR dllPathOrNull) {
    if (g_xray) return true;
    std::wstring name = dllPathOrNull && *dllPathOrNull ? dllPathOrNull : L"xraywrapper.dll";
    g_xray = LoadLibraryW(name.c_str());
    DWORD err = GetLastError();
    char buffer[64];
    sprintf_s(buffer, "Error code: %lu", err);
    MessageBoxA(NULL, buffer, "Error", MB_OK | MB_ICONERROR);
    if (!g_xray) return false;

    auto gp = [&](const char* s){ return GetProcAddress(g_xray, s); };
    pVersion   = reinterpret_cast<fn_Xray_Version  >(gp("Xray_Version"));
    pStart     = reinterpret_cast<fn_Xray_Start    >(gp("Xray_Start"));
    pReload    = reinterpret_cast<fn_Xray_Reload   >(gp("Xray_Reload"));
    pStop      = reinterpret_cast<fn_Xray_Stop     >(gp("Xray_Stop"));
    pLastError = reinterpret_cast<fn_Xray_LastError>(gp("Xray_LastError"));
    pPollLog   = reinterpret_cast<fn_Xray_PollLog  >(gp("Xray_PollLog"));

    if (!pVersion || !pStart || !pReload || !pStop || !pLastError || !pPollLog) {
        FreeLibrary(g_xray); g_xray = nullptr;
        pVersion = nullptr; pStart = nullptr; pReload = nullptr; pStop = nullptr;
        pLastError = nullptr; pPollLog = nullptr;
        return false;
    }
    return true;
}

extern "C" __declspec(dllexport) BOOL __stdcall XH_Init(LPCWSTR dllPathOrNull) {
    return EnsureLoaded(dllPathOrNull) ? TRUE : FALSE;
}

extern "C" __declspec(dllexport) int __stdcall XH_StartJson(const char* json) {
    if (!EnsureLoaded(nullptr)) return 1;
    if (!json) return 1;
    return pStart(json);
}

extern "C" __declspec(dllexport) int __stdcall XH_ReloadJson(const char* json) {
    if (!EnsureLoaded(nullptr)) return 1;
    if (!json) return 1;
    return pReload(json);
}

extern "C" __declspec(dllexport) int __stdcall XH_StartFile(LPCWSTR jsonPath) {
    if (!EnsureLoaded(nullptr)) return 1;
    if (!jsonPath) return 1;
    std::string data = ReadAllBytes(jsonPath);
    if (data.empty()) return 1;
    return pStart(data.c_str());
}

extern "C" __declspec(dllexport) int __stdcall XH_ReloadFile(LPCWSTR jsonPath) {
    if (!EnsureLoaded(nullptr)) return 1;
    if (!jsonPath) return 1;
    std::string data = ReadAllBytes(jsonPath);
    if (data.empty()) return 1;
    return pReload(data.c_str());
}

extern "C" __declspec(dllexport) int __stdcall XH_Stop() {
    if (!EnsureLoaded(nullptr)) return 1;
    return pStop();
}

extern "C" __declspec(dllexport) unsigned __stdcall XH_LastErrorA(char* buf, unsigned cap) {
    if (!EnsureLoaded(nullptr)) return 0;
    if (!buf || cap == 0) return 0;
    return pLastError(buf, cap);
}

extern "C" __declspec(dllexport) unsigned __stdcall XH_PollLogA(char* buf, unsigned cap) {
    if (!EnsureLoaded(nullptr)) return 0;
    if (!buf || cap == 0) return 0;
    return pPollLog(buf, cap);
}

extern "C" __declspec(dllexport) unsigned __stdcall XH_VersionA(char* buf, unsigned cap) {
    if (!EnsureLoaded(nullptr)) return 0;
    if (!buf || cap == 0) return 0;
    const char* v = pVersion();
    unsigned n = 0;
    while (v && v[n] && n + 1 < cap) { buf[n] = v[n]; ++n; }
    if (cap) buf[n < cap ? n : cap - 1] = '\0';
    return n;
}
