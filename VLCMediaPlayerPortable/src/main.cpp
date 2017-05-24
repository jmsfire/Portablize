//VLCMediaPlayerPortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#include <shlobj.h>
#ifdef _WIN64
#include "MinHook/MinHook.h"
#endif

#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 26;        //"\vlc\vlc-qt-interface.ini`"
static wchar_t g_wBuf[MAX_PATH+1];

//-------------------------------------------------------------------------------------------------
EXPORT LPWSTR * WINAPI CommandLineToArgvWStub(LPCWSTR lpCmdLine, int *pNumArgs)
{
    return CommandLineToArgvW(lpCmdLine, pNumArgs);
}

EXPORT HRESULT WINAPI SHGetFolderPathWStub(HWND, int, HANDLE, DWORD, LPWSTR pszPath)
{
    const wchar_t *pSrc = g_wBuf;
    while ((*pszPath++ = *pSrc++));
    return S_OK;
}

//-------------------------------------------------------------------------------------------------
static LONG WINAPI RegCreateKeyExStub(HKEY, LPCVOID, DWORD, LPVOID, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD)
{
    return ERROR_ACCESS_DENIED;
}

static WINBOOL WINAPI SHGetSpecialFolderPathWStub(HWND, LPWSTR pszPath, int, WINBOOL)
{
    const wchar_t *pSrc = g_wBuf;
    while ((*pszPath++ = *pSrc++));
    return TRUE;
}

#ifdef _WIN64
//-------------------------------------------------------------------------------------------------
template <typename T1, typename T2>
static inline bool FCreateHook(T1 *const pTarget, T2 *const pDetour)
{return MH_CreateHook(reinterpret_cast<LPVOID>(pTarget), reinterpret_cast<LPVOID>(pDetour), nullptr) == MH_OK;}
#else
//-------------------------------------------------------------------------------------------------
static bool FPatch(BYTE *const pAddress, size_t szOffset)
{
    constexpr const DWORD dwPatchSize = 1+sizeof(size_t);
    DWORD dwOldProtect;
    if (VirtualProtect(pAddress, dwPatchSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
    {
        szOffset -= reinterpret_cast<size_t>(pAddress) + dwPatchSize;
        const BYTE *pByte = static_cast<const BYTE*>(static_cast<const void*>(&szOffset));
        pAddress[0] = 0xE9;        //jump near
        pAddress[1] = pByte[0];
        pAddress[2] = pByte[1];
        pAddress[3] = pByte[2];
        pAddress[4] = pByte[3];
        DWORD dwTemp;
        if (VirtualProtect(pAddress, dwPatchSize, dwOldProtect, &dwTemp))
            return true;
    }
    return false;
}

template <typename T1, typename T2>
static inline bool FCreateHook(T1 *const pTarget, T2 *const pDetour)
{return FPatch(reinterpret_cast<BYTE*>(pTarget), reinterpret_cast<size_t>(pDetour));}
#endif

//-------------------------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        const DWORD dwLen = GetModuleFileNameW(nullptr, g_wBuf, MAX_PATH+1);
        if (dwLen >= 6 && dwLen < MAX_PATH)
        {
            wchar_t *pDelim = g_wBuf+dwLen;
            do
            {
                if (*--pDelim == L'\\')
                    break;
            } while (pDelim > g_wBuf);
            if (pDelim >= g_wBuf+4 && pDelim <= g_wBuf+MAX_PATH-g_dwPathMargin)
#ifdef _WIN64
                if (MH_Initialize() == MH_OK)
#endif
                    if (
                            FCreateHook(SHGetFolderPathW, SHGetFolderPathWStub) &&
                            FCreateHook(SHGetSpecialFolderPathW, SHGetSpecialFolderPathWStub) &&
                            FCreateHook(RegCreateKeyExA, RegCreateKeyExStub) &&
                            FCreateHook(RegCreateKeyExW, RegCreateKeyExStub))
#ifdef _WIN64
                        if (MH_EnableHook(MH_ALL_HOOKS) == MH_OK)
#endif
                            if (DisableThreadLibraryCalls(hInstDll))
                            {
                                *pDelim = L'\0';
                                return TRUE;
                            }
        }
    }
#ifdef _WIN64
    else if (fdwReason == DLL_PROCESS_DETACH)
        MH_Uninitialize();
#endif
    return FALSE;
}
