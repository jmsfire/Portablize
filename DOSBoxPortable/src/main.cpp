//DOSBoxPortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef _WIN64
#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 25;        //"\DOSBox\dosbox-?.??.conf`"
static char g_cBuf[MAX_PATH+1];

//-------------------------------------------------------------------------------------------------
EXPORT WINBOOL WINAPI SHGetSpecialFolderPathAStub(HWND, LPSTR pszPath, int, WINBOOL)
{
    const char *pSrc = g_cBuf;
    while ((*pszPath++ = *pSrc++));
    return TRUE;
}

//-------------------------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        const DWORD dwLen = GetModuleFileNameA(nullptr, g_cBuf, MAX_PATH+1);
        if (dwLen >= 6 && dwLen < MAX_PATH)
        {
            char *pDelim = g_cBuf+dwLen;
            do
            {
                if (*--pDelim == '\\')
                    break;
            } while (pDelim > g_cBuf);
            if (pDelim >= g_cBuf+4 && pDelim <= g_cBuf+MAX_PATH-g_dwPathMargin && DisableThreadLibraryCalls(hInstDll))
            {
                *pDelim = '\0';
                return TRUE;
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
