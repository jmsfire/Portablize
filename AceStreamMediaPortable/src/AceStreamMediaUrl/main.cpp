//AceStreamMediaUrl
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>

static constexpr const wchar_t *const g_wGuidClass = L"App::4c5ef380-a87f-26a1-3258-7a477de38e06";
static constexpr const ULONGLONG g_iTicksPerMs = 10000;
static constexpr const ULONGLONG g_iEpochDiff = 116444736000000000ULL;

//-------------------------------------------------------------------------------------------------
static inline void FCopyMemoryW(wchar_t *pDst, const wchar_t *pSrc)
{
    while ((*pDst++ = *pSrc++));
}

//-------------------------------------------------------------------------------------------------
static inline bool FIsStartWithW(const wchar_t *pFullStr, const wchar_t *pBeginStr)
{
    while (*pBeginStr)
        if (*pFullStr++ != *pBeginStr++)
            return false;
    return true;
}

//-------------------------------------------------------------------------------------------------
static void FNumToStrW(ULONGLONG iValue, wchar_t *pBuf)
{
    wchar_t *pIt = pBuf;
    do
    {
        *pIt++ = iValue%10 + L'0';
        iValue /= 10;
    } while (iValue > 0);
    *pIt-- = L'\0';
    do
    {
        const wchar_t wTemp = *pIt;
        *pIt = *pBuf;
        *pBuf = wTemp;
    } while (++pBuf < --pIt);
}

//-------------------------------------------------------------------------------------------------
static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HFONT hFont;
    if (uMsg == WM_CREATE)
    {
        NONCLIENTMETRICS nonClientMetrics;
        nonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
        if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &nonClientMetrics, 0) &&
                (nonClientMetrics.lfMessageFont.lfHeight = -12, (hFont = CreateFontIndirectW(&nonClientMetrics.lfMessageFont))))
        {
            const CREATESTRUCT *const crStruct = reinterpret_cast<const CREATESTRUCT*>(lParam);
            if (const HWND hWndEditUrl = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 650, 30, hWnd, nullptr, crStruct->hInstance, nullptr))
            {
                SendMessageW(hWndEditUrl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                SetWindowTextW(hWndEditUrl, static_cast<wchar_t*>(crStruct->lpCreateParams));
                SendMessageW(hWndEditUrl, EM_SETSEL, 0, -1);
                SetFocus(hWndEditUrl);
                return 0;
            }
        }
        return -1;
    }
    if (uMsg == WM_ENDSESSION)
    {
        SendMessageW(hWnd, WM_CLOSE, 0, 0);
        return 0;
    }
    if (uMsg == WM_DESTROY)
    {
        if (hFont)
            DeleteObject(hFont);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

//-------------------------------------------------------------------------------------------------
void FMain()
{
    wchar_t wId[41];
    *wId = L'\0';
    if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(nullptr))
    {
        if (wchar_t *const hClipboardText = static_cast<wchar_t*>(GetClipboardData(CF_UNICODETEXT)))
            if (GlobalLock(hClipboardText))
            {
                wchar_t *wItClp = hClipboardText;
                while (*wItClp == L' ' || *wItClp == L'\t')
                    ++wItClp;
                if (FIsStartWithW(wItClp, L"acestream://"))
                    wItClp += 12;

                wchar_t *wItId = wId;
                const wchar_t *const wEnd = wItClp+40;
                while (wItClp < wEnd)
                    if ((*wItClp >= L'0' && *wItClp <= L'9') || (*wItClp >= L'a' && *wItClp <= L'f'))
                        *wItId++ = *wItClp++;
                    else
                    {
                        *wId = L'\0';
                        break;
                    }
                GlobalUnlock(hClipboardText);
            }
        CloseClipboard();
    }

    if (*wId)
    {
        wchar_t wBuf[MAX_PATH];
        DWORD dwTemp = GetModuleFileNameW(nullptr, wBuf, MAX_PATH+1-4);        //".ini"
        if (dwTemp >= 4 && dwTemp < MAX_PATH-4)        //".ini"
        {
            FCopyMemoryW(wBuf+dwTemp, L".ini");
            const HANDLE hFile = CreateFileW(wBuf, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                dwTemp = 0;
                LARGE_INTEGER iFileSize;
                if (GetFileSizeEx(hFile, &iFileSize) &&
                        iFileSize.HighPart == 0 &&
                        iFileSize.LowPart%sizeof(wchar_t) == 0 &&
                        iFileSize.LowPart >= 9*sizeof(wchar_t) &&        //"1.1.1.1:1"
                        iFileSize.LowPart <= 21*sizeof(wchar_t))        //"200.200.200.200:65535"
                {
                    FCopyMemoryW(wBuf, L"http://");
                    dwTemp = ReadFile(hFile, wBuf+7, iFileSize.LowPart, &dwTemp, nullptr) && dwTemp == iFileSize.LowPart;
                }
                CloseHandle(hFile);

                if (dwTemp)
                {
                    RECT rect; rect.left = 0; rect.top = 0; rect.right = 650; rect.bottom = 30;
                    if (AdjustWindowRectEx(&rect, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~(WS_MAXIMIZEBOX | WS_SIZEBOX), FALSE, 0))
                    {
                        const HINSTANCE hInst = GetModuleHandleW(nullptr);
                        WNDCLASSEX wndCl;
                        wndCl.cbSize = sizeof(WNDCLASSEX);
                        wndCl.style = 0;
                        wndCl.lpfnWndProc = WindowProc;
                        wndCl.cbClsExtra = 0;
                        wndCl.cbWndExtra = 0;
                        wndCl.hInstance = hInst;
                        wndCl.hIcon = nullptr;
                        wndCl.hCursor = LoadCursorW(nullptr, IDC_ARROW);
                        wndCl.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);
                        wndCl.lpszMenuName = nullptr;
                        wndCl.lpszClassName = g_wGuidClass;
                        wndCl.hIconSm = nullptr;

                        if (RegisterClassExW(&wndCl))
                        {
                            RECT rectArea;
                            if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &rectArea, 0))
                            {
                                wId[40] = L'\0';
                                wchar_t *pDelim = wBuf+7+iFileSize.LowPart/sizeof(wchar_t);
                                FCopyMemoryW(pDelim, L"/ace/getstream?id=");
                                pDelim += 18;
                                FCopyMemoryW(pDelim, wId);
                                pDelim += 40;
                                FCopyMemoryW(pDelim, L"&sid=");
                                pDelim += 5;

                                ULONGLONG iUnixTimeSid;
                                GetSystemTimeAsFileTime(static_cast<FILETIME*>(static_cast<void*>(&iUnixTimeSid)));
                                FNumToStrW((iUnixTimeSid-g_iEpochDiff)/g_iTicksPerMs, pDelim);

                                rect.right -= rect.left;
                                rect.bottom -= rect.top;
                                if (CreateWindowExW(0, g_wGuidClass, L"AceStreamMediaUrl", (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~(WS_MAXIMIZEBOX | WS_SIZEBOX),
                                                    (rectArea.right-rectArea.left)/2-rect.right/2, (rectArea.bottom-rectArea.top)/2-rect.bottom/2, rect.right, rect.bottom, nullptr, nullptr, hInst, wBuf))
                                {
                                    MSG msg;
                                    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
                                    {
                                        TranslateMessage(&msg);
                                        DispatchMessageW(&msg);
                                    }
                                }
                            }
                            UnregisterClassW(g_wGuidClass, hInst);
                        }
                    }
                }
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
extern "C" int start()
{
    FMain();
    ExitProcess(0);
    return 0;
}
