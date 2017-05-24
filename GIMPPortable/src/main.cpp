//GIMPPortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 78;        //"\AppData\Local\fontconfig\cache\00000000000000000000000000000000-le64.cache-6`"

//-------------------------------------------------------------------------------------------------
EXPORT LONG WINAPI RegCloseKeyStub(HKEY)
{
    return ERROR_ACCESS_DENIED;
}

EXPORT LONG WINAPI RegOpenKeyExAStub(HKEY, LPCSTR, DWORD, REGSAM, PHKEY)
{
    return ERROR_ACCESS_DENIED;
}

EXPORT LONG WINAPI RegQueryValueExAStub(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD)
{
    return ERROR_ACCESS_DENIED;
}

//-------------------------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        wchar_t wBuf[MAX_PATH+1];
        DWORD dwLen = GetModuleFileNameW(nullptr, wBuf, MAX_PATH+1);
        if (dwLen >= 6 && dwLen < MAX_PATH)
        {
            wchar_t *pDelim = wBuf+dwLen;
            do
            {
                if (*--pDelim == L'\\')
                    break;
            } while (pDelim > wBuf);
            if (pDelim >= wBuf+4 && pDelim <= wBuf+MAX_PATH-g_dwPathMargin &&
                    (*pDelim = L'\0', SetEnvironmentVariableW(L"USERPROFILE", wBuf)) &&
                    DisableThreadLibraryCalls(hInstDll))
                return TRUE;
        }
    }
    return FALSE;
}
