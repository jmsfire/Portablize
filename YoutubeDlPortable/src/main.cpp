//YoutubeDlPortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef _WIN64
#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 66;        //"\.cache\youtube-dl\youtube-sigfuncs\js_en_US-?????????_00.00.json`"
static wchar_t g_wBuf[MAX_PATH+1];

//-------------------------------------------------------------------------------------------------
static inline bool FIsStartWithW(const wchar_t *pFullStr, const wchar_t *pBeginStr)
{
    while (*pBeginStr)
        if (*pFullStr++ != *pBeginStr++)
            return false;
    return true;
}

//-------------------------------------------------------------------------------------------------
EXPORT WINBOOL WINAPI SHGetSpecialFolderPathWStub(HWND, LPWSTR pszPath, int, WINBOOL)
{
    const wchar_t *pSrc = g_wBuf;
    while ((*pszPath++ = *pSrc++));
    return TRUE;
}

//-------------------------------------------------------------------------------------------------
typedef LONG WINAPI (*PRegOpenKeyExW)(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
static PRegOpenKeyExW RegOpenKeyExWReal;
static LONG WINAPI RegOpenKeyExWStub(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
    return (lpSubKey && FIsStartWithW(lpSubKey, L"Software\\Python\\")) ? ERROR_ACCESS_DENIED : RegOpenKeyExWReal(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

//-------------------------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DWORD dwTemp = GetModuleFileNameW(nullptr, g_wBuf, MAX_PATH+1);
        if (dwTemp >= 6 && dwTemp < MAX_PATH)
        {
            wchar_t *pDelim = g_wBuf+dwTemp;
            do
            {
                if (*--pDelim == L'\\')
                    break;
            } while (pDelim > g_wBuf);
            if (pDelim >= g_wBuf+4 && pDelim <= g_wBuf+MAX_PATH-g_dwPathMargin && (*pDelim = L'\0', SetEnvironmentVariableW(L"USERPROFILE", g_wBuf)))
            {
                constexpr const DWORD dwPatchSize = 1+sizeof(size_t);
                BYTE *const pAddress = reinterpret_cast<BYTE*>(RegOpenKeyExW)-dwPatchSize;
                DWORD dwOldProtect;
                if (VirtualProtect(pAddress, dwPatchSize+2, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                {
                    const size_t szOffset = reinterpret_cast<size_t>(RegOpenKeyExWStub)-(reinterpret_cast<size_t>(pAddress) + dwPatchSize);
                    const BYTE *const pByte = static_cast<const BYTE*>(static_cast<const void*>(&szOffset));
                    pAddress[0] = 0xE9;        //jump near
                    pAddress[1] = pByte[0];
                    pAddress[2] = pByte[1];
                    pAddress[3] = pByte[2];
                    pAddress[4] = pByte[3];
                    pAddress[5] = 0xEB;        //jump short
                    pAddress[6] = 0xF9;        //-7
                    if (VirtualProtect(pAddress, dwPatchSize+2, dwOldProtect, &dwTemp) && DisableThreadLibraryCalls(hInstDll))
                    {
                        RegOpenKeyExWReal = reinterpret_cast<PRegOpenKeyExW>(pAddress+dwPatchSize+2);
                        return TRUE;
                    }
                }
            }
        }
    }
    return FALSE;
}
#else
//-------------------------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, DWORD, LPVOID)
{
    return FALSE;
}
#endif
