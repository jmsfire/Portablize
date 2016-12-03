//FileZillaServerPortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#include <shlobj.h>

#ifndef _WIN64
#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 49;        //"\FileZilla Server\FileZilla Server Interface.xml`"
static wchar_t g_wBuf[MAX_PATH+1];
static SIZE_T g_szPathBytes;

//-------------------------------------------------------------------------------------------------
EXPORT HRESULT WINAPI SHGetKnownFolderPathStub(REFKNOWNFOLDERID, DWORD, HANDLE, PWSTR *ppszPath)
{
    if (wchar_t *pMemory = static_cast<wchar_t*>(CoTaskMemAlloc(g_szPathBytes)))
    {
        *ppszPath = pMemory;
        const wchar_t *pSrc = g_wBuf;
        while ((*pMemory++ = *pSrc++));
        return S_OK;
    }
    return S_FALSE;
}

//-------------------------------------------------------------------------------------------------
EXPORT void WINAPI DragFinishStub(HDROP hDrop)
{return DragFinish(hDrop);}
EXPORT UINT WINAPI DragQueryFileWStub(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch)
{return DragQueryFileW(hDrop, iFile, lpszFile, cch);}
EXPORT UINT_PTR WINAPI SHAppBarMessageStub(DWORD dwMessage, PAPPBARDATA pData)
{return SHAppBarMessage(dwMessage, pData);}
EXPORT PIDLIST_ABSOLUTE WINAPI SHBrowseForFolderWStub(LPBROWSEINFOW lpbi)
{return SHBrowseForFolderW(lpbi);}
EXPORT WINBOOL WINAPI Shell_NotifyIconWStub(DWORD dwMessage, PNOTIFYICONDATAW lpData)
{return Shell_NotifyIconW(dwMessage, lpData);}
EXPORT HINSTANCE WINAPI ShellExecuteWStub(HWND hWnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{return ShellExecuteW(hWnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);}
EXPORT HRESULT WINAPI SHGetDesktopFolderStub(IShellFolder **ppshf)
{return SHGetDesktopFolder(ppshf);}
EXPORT DWORD_PTR WINAPI SHGetFileInfoWStub(LPCWSTR pszPath, DWORD dwFileAttributes, SHFILEINFOW *psfi, UINT cbFileInfo, UINT uFlags)
{return SHGetFileInfoW(pszPath, dwFileAttributes, psfi, cbFileInfo, uFlags);}
EXPORT HRESULT WINAPI SHGetMallocStub(IMalloc **ppMalloc)
{return SHGetMalloc(ppMalloc);}
EXPORT WINBOOL WINAPI SHGetPathFromIDListWStub(PCIDLIST_ABSOLUTE pidl, LPWSTR pszPath)
{return SHGetPathFromIDListW(pidl, pszPath);}
EXPORT HRESULT WINAPI SHGetSpecialFolderLocationStub(HWND hWnd, int csidl, PIDLIST_ABSOLUTE *ppidl)
{return SHGetSpecialFolderLocation(hWnd, csidl, ppidl);}

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
            if (pDelim >= g_wBuf+4 && pDelim <= g_wBuf+MAX_PATH-g_dwPathMargin && DisableThreadLibraryCalls(hInstDll))
            {
                *pDelim = L'\0';
                g_szPathBytes = (pDelim-g_wBuf+1)*sizeof(wchar_t);
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
