//qToxPortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#include <ws2tcpip.h>
#include <userenv.h>
#include "MinHook/MinHook.h"

#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 30;        //"\AppData\Roaming\tox\qtox.ini`"
static wchar_t g_wBuf[MAX_PATH+1];
static DWORD g_dwUserProfileLen;
static constexpr const u_short g_iColor = 216*0x101;
static constexpr const wchar_t *const g_wGuidClass = L"App::46e2ea74-50bd-f803-94b8-15b2097756fe";
static HWND g_hWnd;
static bool g_bIsWindowRegister = false;

struct SIpfilter
{
    u_long iIp1;
    u_long iIp2;
};
static SIpfilter *g_pSIpFilter,
*g_pSIpFilterEnd;

//-------------------------------------------------------------------------------------------------
static inline void FCopyMemoryW(wchar_t *pDst, const wchar_t *pSrc)
{
    while ((*pDst++ = *pSrc++));
}

//-------------------------------------------------------------------------------------------------
EXPORT WINBOOL WINAPI SHGetSpecialFolderPathWStub(HWND, LPWSTR pszPath, int, WINBOOL)
{
    FCopyMemoryW(pszPath, g_wBuf);
    return TRUE;
}

EXPORT LPWSTR * WINAPI CommandLineToArgvWStub(LPCWSTR lpCmdLine, int *pNumArgs)
{
    return CommandLineToArgvW(lpCmdLine, pNumArgs);
}

//-------------------------------------------------------------------------------------------------
static WINBOOL WINAPI GetUserProfileDirectoryWStub(HANDLE, LPWSTR lpProfileDir, LPDWORD lpcchSize)
{
    const WINBOOL bOk = lpProfileDir && *lpcchSize >= g_dwUserProfileLen;
    if (bOk)
        FCopyMemoryW(lpProfileDir, g_wBuf);
    *lpcchSize = g_dwUserProfileLen;
    return bOk;
}

//-------------------------------------------------------------------------------------------------
static LONG WINAPI RegCreateKeyExWStub(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD)
{
    return ERROR_ACCESS_DENIED;
}

//-------------------------------------------------------------------------------------------------
static bool FIsBanned(const void *const pSockAddr)
{
    u_long iIp = static_cast<const sockaddr_in*>(pSockAddr)->sin_addr.S_un.S_addr;
    iIp = (iIp << 24) | ((iIp & 0xFF00) << 8) | ((iIp >> 8) & 0xFF00) | (iIp >> 24);
    for (SIpfilter *pIt = g_pSIpFilter; pIt < g_pSIpFilterEnd; ++pIt)
        if (iIp >= pIt->iIp1 && iIp <= pIt->iIp2)
            return true;
    return false;
}

typedef int (WSAAPI *Psendto)(SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen);
static Psendto sendtoReal;
static int WSAAPI sendtoStub(SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen)
{
    return FIsBanned(to) ? WSAEHOSTUNREACH : sendtoReal(s, buf, len, flags, to, tolen);
}

//-------------------------------------------------------------------------------------------------
static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HFONT hFont;

    switch (uMsg)
    {
    case WM_CREATE:
        return ((hFont = CreateFontW(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Tahoma")) &&
                SetLayeredWindowAttributes(hWnd, 0, 0, LWA_COLORKEY)) ? 0 : -1;
    case WM_PAINT:
    {
        static PAINTSTRUCT ps;
        if (const HDC hDc = BeginPaint(hWnd, &ps))
        {
            RECT rect;
            if (GetClientRect(hWnd, &rect))
            {
                rect.right -= 36;
                SelectObject(hDc, hFont);
                SetBkMode(hDc, TRANSPARENT);
                SetTextColor(hDc, RGB(255, 85, 0));
                DrawTextW(hDc, L">>>>", 4, &rect, DT_RIGHT);
            }
            EndPaint(hWnd, &ps);
        }
        return 0;
    }
    case WM_DESTROY:
    {
        if (hFont)
            DeleteObject(hFont);
        g_hWnd = nullptr;
        return 0;
    }
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

static void FInitOsd()
{
    WNDCLASSEXW wndCl;
    wndCl.cbSize = sizeof(WNDCLASSEX);
    wndCl.style = 0;
    wndCl.lpfnWndProc = WindowProc;
    wndCl.cbClsExtra = 0;
    wndCl.cbWndExtra = 0;
    wndCl.hInstance = GetModuleHandleW(nullptr);
    wndCl.hIcon = nullptr;
    wndCl.hCursor = nullptr;
    wndCl.hbrBackground = nullptr;
    wndCl.lpszMenuName = nullptr;
    wndCl.lpszClassName = g_wGuidClass;
    wndCl.hIconSm = nullptr;

    if (RegisterClassExW(&wndCl))
    {
        if ((g_hWnd = CreateWindowExW(WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST, g_wGuidClass, nullptr, WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, wndCl.hInstance, nullptr)))
            g_bIsWindowRegister = true;
        else
            UnregisterClassW(g_wGuidClass, GetModuleHandleW(nullptr));
    }
}

typedef WINBOOL (WINAPI *PFlashWindowEx)(FLASHWINFO *pfwi);
static PFlashWindowEx FlashWindowExReal;
static WINBOOL WINAPI FlashWindowExStub(FLASHWINFO *pfwi)
{
    const WINBOOL bResult = FlashWindowExReal(pfwi);
    static bool bIsInit = false;
    if (!bIsInit)
    {
        bIsInit = true;
        FInitOsd();
    }
    if (g_hWnd)
    {
        if (pfwi->dwFlags == FLASHW_TRAY)
            ShowWindow(g_hWnd, SW_SHOWMAXIMIZED);
        else if (pfwi->dwFlags == FLASHW_STOP)
            ShowWindow(g_hWnd, SW_HIDE);
    }
    return bResult;
}

//-------------------------------------------------------------------------------------------------
typedef VOID (WINAPI *PExitProcess)(UINT uExitCode);
static PExitProcess ExitProcessReal;
static VOID WINAPI ExitProcessStub(UINT uExitCode)
{
    if (g_bIsWindowRegister)
    {
        if (g_hWnd)
            SendMessageW(g_hWnd, WM_CLOSE, 0, 0);
        UnregisterClassW(g_wGuidClass, GetModuleHandleW(nullptr));
    }
    return ExitProcessReal(uExitCode);
}

//-------------------------------------------------------------------------------------------------
struct CQBrush
{
    struct CQBrushData
    {
        enum eBrushStyle {};
        struct CQColor
        {
            enum Spec {};
            Spec cspec;
            u_short alpha;
            u_short red;
            u_short green;
            u_short blue;
        };
        int ref;
        eBrushStyle style;
        CQColor color;
    };
    CQBrushData *data;
};

struct CQString
{
    struct CQStringData
    {
        int ref;
        int size;
        unsigned int alloc : 31;
        unsigned int capacityReserved : 1;
        ptrdiff_t offset;
    };
    const CQStringData *pdata;
    CQStringData data;
    wchar_t wBuf[10];        //": #??????`"
};

enum eCaseSensitivity {eCaseInsensitive, eCaseSensitive};

static CQString g_strBefore;
static CQString g_strAfter;

#ifdef __MINGW32__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#endif
typedef const void * (__thiscall *Preplace)(const void *const this__, const CQString *const strBefore, const CQString *const strAfter, const eCaseSensitivity cs);
static Preplace replacePtr;
typedef void (__thiscall *PsetStyleSheet)(const void *const this__, CQString *const pStrStyleSheet);
static PsetStyleSheet setStyleSheetReal;
static void __thiscall setStyleSheetStub(const void *const this__, CQString *const pStrStyleSheet)
{
    //background
    g_strBefore.data.size = 7;
    FCopyMemoryW(g_strBefore.wBuf, L": white");
    FCopyMemoryW(g_strAfter.wBuf, L": #d8d8d8");
    replacePtr(pStrStyleSheet, &g_strBefore, &g_strAfter, eCaseSensitive);

    //scrollbar normal
    g_strBefore.data.size = 9;
    FCopyMemoryW(g_strBefore.wBuf, L": #d1d1d1");
    FCopyMemoryW(g_strAfter.wBuf, L": #6bc260");
    replacePtr(pStrStyleSheet, &g_strBefore, &g_strAfter, eCaseSensitive);

    //scrollbar hover
    //g_strBefore.data.size = 9;
    FCopyMemoryW(g_strBefore.wBuf, L": #e3e3e3");
    //FCopyMemoryW(g_strAfter.wBuf, L": #6bc260");
    replacePtr(pStrStyleSheet, &g_strBefore, &g_strAfter, eCaseSensitive);

    //scrollbar pressed
    //g_strBefore.data.size = 9;
    FCopyMemoryW(g_strBefore.wBuf, L": #b1b1b1");
    //FCopyMemoryW(g_strAfter.wBuf, L": #6bc260");
    replacePtr(pStrStyleSheet, &g_strBefore, &g_strAfter, eCaseSensitive);

    //textedit hover
    //g_strBefore.data.size = 9;
    FCopyMemoryW(g_strBefore.wBuf, L": #d7d4d1");
    //FCopyMemoryW(g_strAfter.wBuf, L": #6bc260");
    replacePtr(pStrStyleSheet, &g_strBefore, &g_strAfter, eCaseSensitive);

    setStyleSheetReal(this__, pStrStyleSheet);
}

typedef void (__thiscall *PsetBackgroundBrush)(const void *const this__, CQBrush *const brush);
static PsetBackgroundBrush setBackgroundBrushReal;
static void __thiscall setBackgroundBrushStub(const void *const this__, CQBrush *const brush)
{
    CQBrush::CQBrushData::CQColor *const pColor = &brush->data->color;
    pColor->red = g_iColor;
    pColor->green = g_iColor;
    pColor->blue = g_iColor;
    setBackgroundBrushReal(this__, brush);
}
#ifdef __MINGW32__
#pragma GCC diagnostic pop
#endif

//-------------------------------------------------------------------------------------------------
static bool FFillEntry(const char *const cRow, SIpfilter *const pSIpfilter)
{
    if (
            cRow[ 0] >= '0' && cRow[ 0] <= '2' &&
            cRow[ 1] >= '0' && cRow[ 1] <= '9' &&
            cRow[ 2] >= '0' && cRow[ 2] <= '9' &&
            cRow[ 3] == '.' &&
            cRow[ 4] >= '0' && cRow[ 4] <= '2' &&
            cRow[ 5] >= '0' && cRow[ 5] <= '9' &&
            cRow[ 6] >= '0' && cRow[ 6] <= '9' &&
            cRow[ 7] == '.' &&
            cRow[ 8] >= '0' && cRow[ 8] <= '2' &&
            cRow[ 9] >= '0' && cRow[ 9] <= '9' &&
            cRow[10] >= '0' && cRow[10] <= '9' &&
            cRow[11] == '.' &&
            cRow[12] >= '0' && cRow[12] <= '2' &&
            cRow[13] >= '0' && cRow[13] <= '9' &&
            cRow[14] >= '0' && cRow[14] <= '9' &&
            cRow[15] == '-' &&
            cRow[16] >= '0' && cRow[16] <= '2' &&
            cRow[17] >= '0' && cRow[17] <= '9' &&
            cRow[18] >= '0' && cRow[18] <= '9' &&
            cRow[19] == '.' &&
            cRow[20] >= '0' && cRow[20] <= '2' &&
            cRow[21] >= '0' && cRow[21] <= '9' &&
            cRow[22] >= '0' && cRow[22] <= '9' &&
            cRow[23] == '.' &&
            cRow[24] >= '0' && cRow[24] <= '2' &&
            cRow[25] >= '0' && cRow[25] <= '9' &&
            cRow[26] >= '0' && cRow[26] <= '9' &&
            cRow[27] == '.' &&
            cRow[28] >= '0' && cRow[28] <= '2' &&
            cRow[29] >= '0' && cRow[29] <= '9' &&
            cRow[30] >= '0' && cRow[30] <= '9' &&
            cRow[31] == '\n')
    {
        const u_long
                iOctet1A = (cRow[ 0]-'0')*100 + (cRow[ 1]-'0')*10 + (cRow[ 2]-'0'),
                iOctet1B = (cRow[ 4]-'0')*100 + (cRow[ 5]-'0')*10 + (cRow[ 6]-'0'),
                iOctet1C = (cRow[ 8]-'0')*100 + (cRow[ 9]-'0')*10 + (cRow[10]-'0'),
                iOctet1D = (cRow[12]-'0')*100 + (cRow[13]-'0')*10 + (cRow[14]-'0'),
                iOctet2A = (cRow[16]-'0')*100 + (cRow[17]-'0')*10 + (cRow[18]-'0'),
                iOctet2B = (cRow[20]-'0')*100 + (cRow[21]-'0')*10 + (cRow[22]-'0'),
                iOctet2C = (cRow[24]-'0')*100 + (cRow[25]-'0')*10 + (cRow[26]-'0'),
                iOctet2D = (cRow[28]-'0')*100 + (cRow[29]-'0')*10 + (cRow[30]-'0');
        if (iOctet1A <= 255 && iOctet1B <= 255 && iOctet1C <= 255 && iOctet1D <= 255 &&
                iOctet2A <= 255 && iOctet2B <= 255 && iOctet2C <= 255 && iOctet2D <= 255 &&
                (pSIpfilter->iIp1 = (iOctet1A << 24) | (iOctet1B << 16) | (iOctet1C << 8) | iOctet1D) <=
                (pSIpfilter->iIp2 = (iOctet2A << 24) | (iOctet2B << 16) | (iOctet2C << 8) | iOctet2D))
            return true;
    }
    return false;
}

//-------------------------------------------------------------------------------------------------
template <typename T>
static inline bool FCreateHook(T *const pTarget, T *const pDetour, T **const ppOriginal = nullptr)
{return MH_CreateHook(reinterpret_cast<LPVOID>(pTarget), reinterpret_cast<LPVOID>(pDetour), reinterpret_cast<LPVOID*>(ppOriginal)) == MH_OK;}

//-------------------------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        if (HMODULE hMod = GetModuleHandleW(L"Qt5Widgets.dll"))
            if (const PsetStyleSheet setStyleSheetPtr = reinterpret_cast<PsetStyleSheet>(GetProcAddress(hMod, "_ZN7QWidget13setStyleSheetERK7QString")))
                //if (const PgetSaveFileName getSaveFileNamePtr = reinterpret_cast<PgetSaveFileName>(GetProcAddress(hMod, "_ZN11QFileDialog15getSaveFileNameEP7QWidgetRK7QStringS4_S4_PS2_6QFlagsINS_6OptionEE")))
                if (const PsetBackgroundBrush setBackgroundBrushPtr = reinterpret_cast<PsetBackgroundBrush>(GetProcAddress(hMod, "_ZN13QGraphicsView18setBackgroundBrushERK6QBrush")))
                    if ((hMod = GetModuleHandleW(L"Qt5Core.dll")) &&
                            (replacePtr = reinterpret_cast<Preplace>(GetProcAddress(hMod, "_ZN7QString7replaceERKS_S1_N2Qt15CaseSensitivityE"))))
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
                            if (pDelim >= g_wBuf+4 && pDelim <= g_wBuf+MAX_PATH-g_dwPathMargin)
                                if (const HANDLE hProcHeap = GetProcessHeap())
                                {
                                    FCopyMemoryW(pDelim+1, L"ipfilter.cfg");
                                    const HANDLE hFile = CreateFileW(g_wBuf, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                                    if (hFile != INVALID_HANDLE_VALUE)
                                    {
                                        //32 = "000.000.000.000-255.255.255.255`"
                                        LARGE_INTEGER iFileSize;
                                        if (GetFileSizeEx(hFile, &iFileSize) && iFileSize.HighPart == 0 && iFileSize.LowPart <= 16*1024*1024 && iFileSize.LowPart%32 == 0)
                                        {
                                            if (char *const cFile = static_cast<char*>(HeapAlloc(hProcHeap, HEAP_NO_SERIALIZE, iFileSize.LowPart)))
                                            {
                                                bool bOk = ReadFile(hFile, cFile, iFileSize.LowPart, &dwTemp, nullptr) && dwTemp == iFileSize.LowPart;
                                                CloseHandle(hFile);
                                                if (bOk)
                                                {
                                                    dwTemp = iFileSize.LowPart/32;
                                                    if ((g_pSIpFilter = static_cast<SIpfilter*>(HeapAlloc(hProcHeap, HEAP_NO_SERIALIZE, dwTemp*sizeof(SIpfilter)))))
                                                    {
                                                        g_pSIpFilterEnd = g_pSIpFilter + dwTemp;
                                                        const char *cIt = cFile;
                                                        for (SIpfilter *pIt = g_pSIpFilter; pIt < g_pSIpFilterEnd; ++pIt, cIt += 32)
                                                            if (!FFillEntry(cIt, pIt))
                                                            {
                                                                bOk = false;
                                                                break;
                                                            }
                                                    }
                                                }
                                                if (HeapFree(hProcHeap, HEAP_NO_SERIALIZE, cFile) &&
                                                        bOk &&
                                                        MH_Initialize() == MH_OK &&
                                                        FCreateHook(GetUserProfileDirectoryW, GetUserProfileDirectoryWStub) &&
                                                        FCreateHook(RegCreateKeyExW, RegCreateKeyExWStub) &&
                                                        (g_pSIpFilter == g_pSIpFilterEnd || FCreateHook(sendto, sendtoStub, &sendtoReal)) &&
                                                        FCreateHook(FlashWindowEx, FlashWindowExStub, &FlashWindowExReal) &&
                                                        FCreateHook(ExitProcess, ExitProcessStub, &ExitProcessReal) &&
                                                        FCreateHook(setStyleSheetPtr, setStyleSheetStub, &setStyleSheetReal) &&
                                                        //FCreateHook(getSaveFileNamePtr, getSaveFileNameStub, &getSaveFileNameReal) &&
                                                        FCreateHook(setBackgroundBrushPtr, setBackgroundBrushStub, &setBackgroundBrushReal) &&
                                                        MH_EnableHook(MH_ALL_HOOKS) == MH_OK &&
                                                        DisableThreadLibraryCalls(hInstDll))
                                                {
                                                    *pDelim = L'\0';
                                                    g_dwUserProfileLen = pDelim-g_wBuf+1;

                                                    g_strBefore.data.ref = 1;
                                                    //g_strBefore.data.size = ?;
                                                    g_strBefore.data.alloc = sizeof(CQString::wBuf);
                                                    g_strBefore.data.capacityReserved = 0;
                                                    g_strBefore.data.offset = sizeof(CQString::CQStringData);
                                                    g_strBefore.pdata = &g_strBefore.data;
                                                    g_strAfter.data.ref = 1;
                                                    g_strAfter.data.size = 9;
                                                    g_strAfter.data.alloc = sizeof(CQString::wBuf);
                                                    g_strAfter.data.capacityReserved = 0;
                                                    g_strAfter.data.offset = sizeof(CQString::CQStringData);
                                                    g_strAfter.pdata = &g_strAfter.data;
                                                    return TRUE;
                                                }
                                            }
                                            else
                                                CloseHandle(hFile);
                                        }
                                        else
                                            CloseHandle(hFile);
                                    }
                                }
                        }
                    }
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        if (g_pSIpFilter)
            if (const HANDLE hProcHeap = GetProcessHeap())
                HeapFree(hProcHeap, HEAP_NO_SERIALIZE, g_pSIpFilter);
        MH_Uninitialize();
    }
    return FALSE;
}
