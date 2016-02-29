#include <windows.h>
#include <shlobj.h>
#include <wininet.h>

#define EXPORT //__declspec(dllexport)

enum
{
    eSize = 1+sizeof(size_t),
    eLen = 47,        //[47 = "\CrystalIdea Software\SpeedyFox\preferences.xml"]
    eIdAvail = 1004,
    eIdMoreSoft = 1009,
    eIdLogo = 1026,
    eIdSysTree = 1012
};

static wchar_t g_wBuf[MAX_PATH+1];
static DWORD g_dwCurrentThreadId;

//-------------------------------------------------------------------------------------------------
WINBOOL WINAPI SHGetSpecialFolderPathWStub(HWND, LPWSTR pszPath, int, WINBOOL)
{
    wcscpy(pszPath, g_wBuf);
    return TRUE;
}

//-------------------------------------------------------------------------------------------------
VOID CALLBACK TimerProc(HWND, UINT, UINT_PTR idEvent, DWORD)
{
    HWND hWnd = 0;
    while ((hWnd = FindWindowEx(0, hWnd, L"#32770", 0)))
        if (GetWindowThreadProcessId(hWnd, 0) == g_dwCurrentThreadId)
        {
            if (KillTimer(0, idEvent))
                if (HWND hWndTemp = GetDlgItem(hWnd, eIdAvail))
                    if (ShowWindow(hWndTemp, SW_HIDE) &&
                            (hWndTemp = GetDlgItem(hWnd, eIdMoreSoft)) &&
                            ShowWindow(hWndTemp, SW_HIDE) &&
                            (hWndTemp = GetDlgItem(hWnd, eIdLogo)) &&
                            ShowWindow(hWndTemp, SW_HIDE) &&
                            (hWndTemp = FindWindowEx(hWnd, 0, L"Static", L"")) &&
                            (hWndTemp = FindWindowEx(hWnd, hWndTemp, L"Static", L"")) &&
                            ShowWindow(hWndTemp, SW_HIDE) &&
                            (hWndTemp = GetDlgItem(hWnd, eIdSysTree)))
                    {
                        RECT rect;
                        if (GetClientRect(hWndTemp, &rect))
                        {
                            const LONG iWidth = rect.right;
                            if (GetClientRect(hWnd, &rect) && SetWindowPos(hWndTemp, HWND_TOP, 0, 0, iWidth, rect.bottom, SWP_NOZORDER))
                                InvalidateRect(hWndTemp, 0, FALSE);
                        }
                    }
            break;
        }
}

//-------------------------------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH && (g_dwCurrentThreadId = GetCurrentThreadId()))
    {
        DWORD dwTemp = GetModuleFileName(0, g_wBuf, MAX_PATH+1);
        if (dwTemp >= 18/*C:\A\speedyfox.exe*/ && dwTemp < MAX_PATH)
            if (wchar_t *const pDelim = wcsrchr(g_wBuf, L'\\'))
                if (pDelim >= g_wBuf+4 && pDelim <= g_wBuf + MAX_PATH-1-eLen)
                {
                    BYTE *const pAddress = reinterpret_cast<BYTE*>(SHGetSpecialFolderPathW);
                    DWORD dwOldProtect;
                    if (VirtualProtect(pAddress, eSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                    {
                        *pAddress = 0xE9/*jump near*/;
                        const size_t szOffset = reinterpret_cast<size_t>(SHGetSpecialFolderPathWStub) - (reinterpret_cast<size_t>(pAddress) + eSize);
                        memcpy(pAddress+1, &szOffset, sizeof(size_t));
                        if (VirtualProtect(pAddress, eSize, dwOldProtect, &dwTemp) && SetTimer(0, 1, 200, TimerProc))
                        {
                            *pDelim = L'\0';
                            return TRUE;
                        }
                    }
                }
    }
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
EXPORT INTERNET_STATUS_CALLBACK InternetSetStatusCallbackWStub(HINTERNET, INTERNET_STATUS_CALLBACK) {return 0;}
EXPORT BOOL InternetGetLastResponseInfoWStub(LPDWORD, LPWSTR, LPDWORD) {return FALSE;}
EXPORT BOOL InternetSetOptionExWStub(HINTERNET, DWORD, LPVOID, DWORD, DWORD) {return FALSE;}
EXPORT BOOL InternetQueryOptionWStub(HINTERNET, DWORD, LPVOID, LPDWORD) {return FALSE;}
EXPORT BOOL InternetCloseHandleStub(HINTERNET) {return FALSE;}
EXPORT BOOL InternetQueryDataAvailableStub(HINTERNET, LPDWORD, DWORD, DWORD_PTR) {return FALSE;}
EXPORT BOOL InternetWriteFileStub(HINTERNET, LPCVOID, DWORD, LPDWORD) {return FALSE;}
EXPORT DWORD InternetSetFilePointerStub(HINTERNET, LONG, PVOID, DWORD, DWORD_PTR) {return INVALID_SET_FILE_POINTER;}
EXPORT BOOL InternetReadFileStub(HINTERNET, LPVOID, DWORD, LPDWORD) {return FALSE;}
EXPORT HINTERNET InternetOpenUrlWStub(HINTERNET, LPCWSTR, LPCWSTR, DWORD, DWORD, DWORD_PTR) {return 0;}
EXPORT BOOL InternetCrackUrlWStub(LPCWSTR, DWORD, DWORD, LPURL_COMPONENTSW) {return FALSE;}
EXPORT BOOL InternetCanonicalizeUrlWStub(LPCWSTR, LPWSTR, LPDWORD, DWORD) {return FALSE;}
EXPORT HINTERNET InternetOpenWStub(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD) {return 0;}
