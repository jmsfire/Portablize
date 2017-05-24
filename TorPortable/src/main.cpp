//TorPortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#define WIN32_LEAN_AND_MEAN
#include <shlobj.h>

#ifndef _WIN64
#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 32;        //"\tor\cached-microdesc-consensus`"
static wchar_t g_wBuf[MAX_PATH+1];

//-------------------------------------------------------------------------------------------------
EXPORT HRESULT WINAPI SHGetMallocStub(IMalloc **ppMalloc)
{
    return SHGetMalloc(ppMalloc);
}

EXPORT WINBOOL WINAPI SHGetPathFromIDListAStub(PCIDLIST_ABSOLUTE pidl, LPSTR pszPath)
{
    return SHGetPathFromIDListA(pidl, pszPath);
}

EXPORT HRESULT WINAPI SHGetSpecialFolderLocationStub(HWND, int, PIDLIST_ABSOLUTE *ppidl)
{
    return SHILCreateFromPath(g_wBuf, ppidl, nullptr);
}

//-------------------------------------------------------------------------------------------------
static VOID CALLBACK TimerAPCProc(LPVOID lpArgToCompletionRoutine, DWORD, DWORD)
{
    if (const HWND hWnd = GetConsoleWindow())
        if (const HANDLE hIcon = LoadImageW(static_cast<HINSTANCE>(lpArgToCompletionRoutine), L"IDI_ICON1", IMAGE_ICON, 16, 16, 0))
            SendMessageW(hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
}

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
            if (pDelim >= g_wBuf+4 && pDelim <= g_wBuf+MAX_PATH-g_dwPathMargin &&
                    (pDelim[1] = L'\0', SetCurrentDirectoryW(g_wBuf)) &&
                    DisableThreadLibraryCalls(hInstDll))
                if (const HANDLE hTimer = CreateWaitableTimerW(nullptr, FALSE, nullptr))
                {
                    LARGE_INTEGER liDueTime;
                    liDueTime.QuadPart = 0;
                    const bool bOk = SetWaitableTimer(hTimer, &liDueTime, 0, TimerAPCProc, hInstDll, FALSE);
                    if (CloseHandle(hTimer) && bOk)
                    {
                        *pDelim = L'\0';
                        return TRUE;
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
