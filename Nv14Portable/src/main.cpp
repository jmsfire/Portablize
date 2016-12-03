//Nv14Portable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#define WIN32_LEAN_AND_MEAN
#include <shlobj.h>

#ifndef _WIN64
#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 84;        //"\Macromedia\Flash Player\macromedia.com\support\flashplayer\sys\#local\settings.sol`"
static char g_cBuf[MAX_PATH+1];

//-------------------------------------------------------------------------------------------------
static UINT WINAPI GetSystemDirectoryAStub(LPSTR lpBuffer, UINT)
{
    const char *pSrc = g_cBuf;
    while ((*lpBuffer++ = *pSrc++));
    return pSrc-g_cBuf-1;
}

//-------------------------------------------------------------------------------------------------
EXPORT LONG WINAPI RegCloseKeyStub(HKEY hKey)
{
    return RegCloseKey(hKey);
}

EXPORT LONG WINAPI RegCreateKeyAStub(HKEY, LPCSTR, PHKEY)
{
    return ERROR_ACCESS_DENIED;
}

EXPORT LONG WINAPI RegOpenKeyExAStub(HKEY, LPCSTR, DWORD, REGSAM, PHKEY)
{
    return ERROR_SUCCESS;
}

EXPORT LONG WINAPI RegQueryValueExAStub(HKEY, LPCSTR, LPDWORD, LPDWORD, /*LPBYTE*/char *lpData, LPDWORD)
{
    const char *pSrc = g_cBuf;
    while ((*lpData++ = *pSrc++));
    return ERROR_SUCCESS;
}

EXPORT LONG WINAPI RegSetValueAStub(HKEY, LPCSTR, DWORD, LPCSTR, DWORD)
{
    return ERROR_ACCESS_DENIED;
}

EXPORT LONG WINAPI RegSetValueExAStub(HKEY, LPCSTR, DWORD, DWORD, CONST BYTE*, DWORD)
{
    return ERROR_ACCESS_DENIED;
}

//-------------------------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        constexpr const DWORD dwPatchSize = 1+sizeof(size_t);
        DWORD dwTemp = GetModuleFileNameA(nullptr, g_cBuf, MAX_PATH+1);
        if (dwTemp >= 6 && dwTemp < MAX_PATH)
        {
            char *pDelim = g_cBuf+dwTemp;
            do
            {
                if (*--pDelim == '\\')
                    break;
            } while (pDelim > g_cBuf);
            if (pDelim >= g_cBuf+4 && pDelim <= g_cBuf+MAX_PATH-g_dwPathMargin)
            {
                BYTE *const pAddress = reinterpret_cast<BYTE*>(GetSystemDirectoryA);
                DWORD dwOldProtect;
                if (VirtualProtect(pAddress, dwPatchSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                {
                    const size_t szOffset = reinterpret_cast<size_t>(GetSystemDirectoryAStub) - (reinterpret_cast<size_t>(pAddress) + dwPatchSize);
                    const BYTE *const pByte = static_cast<const BYTE*>(static_cast<const void*>(&szOffset));
                    pAddress[0] = 0xE9;        //jump near
                    pAddress[1] = pByte[0];
                    pAddress[2] = pByte[1];
                    pAddress[3] = pByte[2];
                    pAddress[4] = pByte[3];
                    if (VirtualProtect(pAddress, dwPatchSize, dwOldProtect, &dwTemp) && DisableThreadLibraryCalls(hInstDll))
                    {
                        *pDelim = '\0';
                        return TRUE;
                    }
                }
            }
        }
    }
    //restore kernel32!GetSystemDirectoryA supposedly don't needed
    return FALSE;
}
#else
//-------------------------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, DWORD, LPVOID)
{
    return FALSE;
}
#endif
