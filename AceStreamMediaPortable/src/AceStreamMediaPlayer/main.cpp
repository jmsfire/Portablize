//AceStreamMediaPlayer
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static constexpr const ULONGLONG g_iTicksPerMs = 10000;
static constexpr const ULONGLONG g_iEpochDiff = 116444736000000000ULL;

//-------------------------------------------------------------------------------------------------
static inline void FCopyMemoryW(wchar_t *pDst, const wchar_t *pSrc)
{
    while ((*pDst++ = *pSrc++));
}

//-------------------------------------------------------------------------------------------------
static inline bool FCompareMemoryW(const wchar_t *pBuf1, const wchar_t *pBuf2, DWORD dwSize)
{
    while (dwSize--)
        if (*pBuf1++ != *pBuf2++)
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
                if (FCompareMemoryW(wItClp, L"acestream://", 12))
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
        wchar_t wBuf[MAX_PATH*2];
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
                        iFileSize.LowPart &&
                        iFileSize.LowPart/sizeof(wchar_t) < MAX_PATH*2-102)//" http://127.0.0.1:6878/ace/getstream?id=0000000000000000000000000000000000000000&sid=1833029933770955`"
                    dwTemp = ReadFile(hFile, wBuf, iFileSize.LowPart, &dwTemp, nullptr) && dwTemp == iFileSize.LowPart;
                CloseHandle(hFile);

                if (dwTemp)
                {
                    wId[40] = L'\0';
                    wchar_t *pDelim = wBuf+iFileSize.LowPart/sizeof(wchar_t);
                    FCopyMemoryW(pDelim, L" http://127.0.0.1:6878/ace/getstream?id=");
                    pDelim += 40;
                    FCopyMemoryW(pDelim, wId);
                    pDelim += 40;
                    FCopyMemoryW(pDelim, L"&sid=");
                    pDelim += 5;

                    ULONGLONG iUnixTimeSid;
                    GetSystemTimeAsFileTime(static_cast<FILETIME*>(static_cast<void*>(&iUnixTimeSid)));
                    FNumToStrW((iUnixTimeSid-g_iEpochDiff)/g_iTicksPerMs, pDelim);

                    PROCESS_INFORMATION pi;
                    STARTUPINFO si;
                    BYTE *pDst = static_cast<BYTE*>(static_cast<void*>(&si));
                    DWORD dwSize = sizeof(STARTUPINFO);
                    while (dwSize--)
                        *pDst++ = '\0';
                    si.cb = sizeof(STARTUPINFO);
                    if (CreateProcessW(nullptr, wBuf, nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, &si, &pi))
                    {
                        CloseHandle(pi.hThread);
                        CloseHandle(pi.hProcess);
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
