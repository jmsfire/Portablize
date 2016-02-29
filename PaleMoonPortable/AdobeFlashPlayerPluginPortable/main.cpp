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
    eLen = 83        //[83 = "\Macromedia\Flash Player\macromedia.com\support\flashplayer\sys\#local\settings.sol"]
};

static wchar_t g_wBuf[MAX_PATH+1];

//-------------------------------------------------------------------------------------------------
HRESULT WINAPI SHGetFolderPathWStub(HWND, int, HANDLE, DWORD, LPWSTR pszPath)
{
    wcscpy(pszPath, g_wBuf);
    return S_OK;
}

//-------------------------------------------------------------------------------------------------
UINT WINAPI GetSystemWow64DirectoryWStub(LPWSTR lpBuffer, UINT)
{
    wcscpy(lpBuffer, g_wBuf);
    return 1;        //only ok checked
    //return wcslen(g_wBuf);
}

LONG WINAPI RegCreateKeyExAStub(HKEY, LPCSTR, DWORD, LPSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD)
{
    return ERROR_ACCESS_DENIED;
}

//-------------------------------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DWORD dwTemp = GetModuleFileName(0, g_wBuf, MAX_PATH+1);
        if (dwTemp >= 25/*C:\A\plugin-container.exe*/ && dwTemp < MAX_PATH)
            if (wchar_t *const pDelim = wcsrchr(g_wBuf, L'\\'))
                if (pDelim >= g_wBuf+4 && pDelim <= g_wBuf + MAX_PATH-1-eLen)
                {
#ifdef _WIN64
                    if (MH_Initialize() == MH_OK)
                    {
                        if (MH_CreateHookApi(L"shell32.dll", "SHGetFolderPathW", reinterpret_cast<LPVOID>(SHGetFolderPathWStub), 0) == MH_OK &&
                                MH_EnableHook(reinterpret_cast<LPVOID>(SHGetFolderPathW)) == MH_OK)
                        {
                            if (MH_CreateHookApi(L"kernel32.dll", "GetSystemWow64DirectoryW", reinterpret_cast<LPVOID>(GetSystemWow64DirectoryWStub), 0) == MH_OK &&
                                    MH_EnableHook(reinterpret_cast<LPVOID>(GetSystemWow64DirectoryW)) == MH_OK)
                            {
                                if (MH_CreateHookApi(L"advapi32.dll", "RegCreateKeyExA", reinterpret_cast<LPVOID>(RegCreateKeyExAStub), 0) == MH_OK &&
                                        MH_EnableHook(reinterpret_cast<LPVOID>(RegCreateKeyExA)) == MH_OK)
                                {
                                    *pDelim = L'\0';
                                    return TRUE;
                                }
                                MH_DisableHook(reinterpret_cast<LPVOID>(GetSystemWow64DirectoryW));
                            }
                            MH_DisableHook(reinterpret_cast<LPVOID>(SHGetFolderPathWStub));
                        }
                        MH_Uninitialize();
                    }
#else
                    BYTE *pAddress = reinterpret_cast<BYTE*>(SHGetFolderPathW);
                    DWORD dwOldProtect;
                    if (VirtualProtect(pAddress, eSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                    {
                        *pAddress = 0xE9/*jump near*/;
                        size_t szOffset = reinterpret_cast<size_t>(SHGetFolderPathWStub) - (reinterpret_cast<size_t>(pAddress) + eSize);
                        memcpy(pAddress+1, &szOffset, sizeof(size_t));
                        if (VirtualProtect(pAddress, eSize, dwOldProtect, &dwTemp))
                        {
                            pAddress = reinterpret_cast<BYTE*>(GetSystemWow64DirectoryW);
                            if (VirtualProtect(pAddress, eSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                            {
                                *pAddress = 0xE9/*jump near*/;
                                szOffset = reinterpret_cast<size_t>(GetSystemWow64DirectoryWStub) - (reinterpret_cast<size_t>(pAddress) + eSize);
                                memcpy(pAddress+1, &szOffset, sizeof(size_t));
                                if (VirtualProtect(pAddress, eSize, dwOldProtect, &dwTemp))
                                {
                                    pAddress = reinterpret_cast<BYTE*>(RegCreateKeyExA);
                                    if (VirtualProtect(pAddress, eSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                                    {
                                        *pAddress = 0xE9/*jump near*/;
                                        szOffset = reinterpret_cast<size_t>(RegCreateKeyExAStub) - (reinterpret_cast<size_t>(pAddress) + eSize);
                                        memcpy(pAddress+1, &szOffset, sizeof(size_t));
                                        if (VirtualProtect(pAddress, eSize, dwOldProtect, &dwTemp))
                                        {
                                            *pDelim = L'\0';
                                            return TRUE;
                                        }
                                    }
                                }
                            }
                        }
                    }
#endif
                }
    }
#ifdef _WIN64
    else if (fdwReason == DLL_PROCESS_ATTACH)
    {
        MH_DisableHook(reinterpret_cast<LPVOID>(RegCreateKeyExA));
        MH_DisableHook(reinterpret_cast<LPVOID>(GetSystemWow64DirectoryW));
        MH_DisableHook(reinterpret_cast<LPVOID>(SHGetFolderPathW));
        MH_Uninitialize();
    }
#endif
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
EXPORT HRESULT WINAPI DirectInput8CreateStub(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN) {return S_FALSE;}
