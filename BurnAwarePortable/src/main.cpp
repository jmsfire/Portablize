//BurnAwarePortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef _WIN64
#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 35;        //"\Burnaware\BurnAware\burnaware.ini`"
static wchar_t g_wBuf[MAX_PATH+1];

//-------------------------------------------------------------------------------------------------
EXPORT HRESULT WINAPI SHGetFolderPathWStub(HWND, int, HANDLE, DWORD, LPWSTR pszPath)
{
    const wchar_t *pSrc = g_wBuf;
    while ((*pszPath++ = *pSrc++));
    return S_OK;
}

//-------------------------------------------------------------------------------------------------
static LONG WINAPI RegOpenKeyExWStub(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY)
{
    return ERROR_ACCESS_DENIED;
}

//-------------------------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        constexpr const DWORD dwPatchSize = 1+sizeof(size_t);
        DWORD dwTemp = GetModuleFileNameW(nullptr, g_wBuf, MAX_PATH+1);
        if (dwTemp >= 6 && dwTemp < MAX_PATH)
        {
            wchar_t *pDelim = g_wBuf+dwTemp;
            do
            {
                if (*--pDelim == L'\\')
                    break;
            } while (pDelim > g_wBuf);
            if (pDelim >= g_wBuf+4 && pDelim <= g_wBuf+MAX_PATH-g_dwPathMargin)
            {
                BYTE *const pAddress = reinterpret_cast<BYTE*>(RegOpenKeyExW);
                DWORD dwOldProtect;
                if (VirtualProtect(pAddress, dwPatchSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                {
                    const size_t szOffset = reinterpret_cast<size_t>(RegOpenKeyExWStub) - (reinterpret_cast<size_t>(pAddress) + dwPatchSize);
                    const BYTE *const pByte = static_cast<const BYTE*>(static_cast<const void*>(&szOffset));
                    pAddress[0] = 0xE9;        //jump near
                    pAddress[1] = pByte[0];
                    pAddress[2] = pByte[1];
                    pAddress[3] = pByte[2];
                    pAddress[4] = pByte[3];
                    if (VirtualProtect(pAddress, dwPatchSize, dwOldProtect, &dwTemp) && DisableThreadLibraryCalls(hInstDll))
                    {
                        *pDelim = L'\0';
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
