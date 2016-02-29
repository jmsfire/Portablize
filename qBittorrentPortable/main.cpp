//it's use MinHook library for x64 version:
//"http://www.codeproject.com/Articles/44326/MinHook-The-Minimalistic-x-x-API-Hooking-Libra"
#include <windows.h>
#include <shlobj.h>
#ifdef _WIN64
#include "MinHook/include/MinHook.h"
#endif

#define EXPORT //__declspec(dllexport)

enum
{
    eSize = 1+sizeof(size_t),
    eLen = 76        //[76 = "\qBittorrent\BT_backup\0123456789abcdefghijklmnopqrstuvwxyz????.fastresume.0"]
};

typedef BOOLEAN WINAPI (*PSetSuspendStatePointer)(BOOLEAN Hibernate, BOOLEAN ForceCritical, BOOLEAN DisableWakeEvent);
static PSetSuspendStatePointer SetSuspendStatePointer;
static wchar_t g_wBuf[MAX_PATH+1];
static HMODULE g_hMod;

//-------------------------------------------------------------------------------------------------
BOOLEAN WINAPI SetSuspendStateNull(BOOLEAN, BOOLEAN, BOOLEAN) {return 0;}

//-------------------------------------------------------------------------------------------------
EXPORT BOOLEAN WINAPI SetSuspendStateStub(BOOLEAN Hibernate, BOOLEAN ForceCritical, BOOLEAN DisableWakeEvent)
{
    if (!SetSuspendStatePointer)
    {
        wchar_t wBuf[33/*C:\Windows\System32\powrprof.dll`*/];
        if (!(GetSystemDirectory(wBuf, 20) == 19 &&
              (wcscpy(wBuf+19, L"\\powrprof.dll"), (g_hMod = LoadLibrary(wBuf))) &&
              (SetSuspendStatePointer = reinterpret_cast<PSetSuspendStatePointer>(GetProcAddress(g_hMod, "SetSuspendState")))))
        {
            SetSuspendStatePointer = SetSuspendStateNull;
            if (g_hMod)
            {
                FreeLibrary(g_hMod);
                g_hMod = 0;
            }
        }
    }
    return SetSuspendStatePointer(Hibernate, ForceCritical, DisableWakeEvent);
}

//-------------------------------------------------------------------------------------------------
WINBOOL WINAPI SHGetSpecialFolderPathWStub(HWND, LPWSTR pszPath, int, WINBOOL)
{
    wcscpy(pszPath, g_wBuf);
    return TRUE;
}

//-------------------------------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DWORD dwTemp = GetModuleFileName(0, g_wBuf, MAX_PATH+1);
        if (dwTemp >= 20/*C:\A\qbittorrent.exe*/ && dwTemp < MAX_PATH)
            if (wchar_t *const pDelim = wcsrchr(g_wBuf, L'\\'))
                if (pDelim >= g_wBuf+4 && pDelim <= g_wBuf + MAX_PATH-1-eLen)
                {
#ifdef _WIN64
                    if (MH_Initialize() == MH_OK)
                    {
                        if (MH_CreateHookApi(L"shell32.dll", "SHGetSpecialFolderPathW", reinterpret_cast<LPVOID>(SHGetSpecialFolderPathWStub), 0) == MH_OK &&
                                MH_EnableHook(reinterpret_cast<LPVOID>(SHGetSpecialFolderPathW)) == MH_OK)
                        {
                            *pDelim = L'\0';
                            return TRUE;
                        }
                        MH_Uninitialize();
                    }
#else
                    BYTE *const pAddress = reinterpret_cast<BYTE*>(SHGetSpecialFolderPathW);
                    DWORD dwOldProtect;
                    if (VirtualProtect(pAddress, eSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                    {
                        *pAddress = 0xE9/*jump near*/;
                        const size_t szOffset = reinterpret_cast<size_t>(SHGetSpecialFolderPathWStub) - (reinterpret_cast<size_t>(pAddress) + eSize);
                        memcpy(pAddress+1, &szOffset, sizeof(size_t));
                        if (VirtualProtect(pAddress, eSize, dwOldProtect, &dwTemp))
                        {
                            *pDelim = L'\0';
                            return TRUE;
                        }
                    }
#endif
                }
    }
    else if (fdwReason == DLL_PROCESS_ATTACH)
    {
#ifdef _WIN64
        MH_DisableHook(reinterpret_cast<LPVOID>(SHGetSpecialFolderPathW));
        MH_Uninitialize();
#endif
        if (g_hMod)
            FreeLibrary(g_hMod);
    }
    return FALSE;
}
