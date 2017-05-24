//SkypePortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#include <ws2tcpip.h>
#include <wininet.h>
#include <shlobj.h>

#ifndef _WIN64
#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 70;        //"\AppData\Local\Microsoft\Windows\Explorer\iconcache_wide_alternate.db`"
static constexpr const DWORD g_dwMaxDomains = 256;
static constexpr const DWORD g_dwPatchSize = 1+sizeof(size_t);
static constexpr const LONG g_iKeyDummy = 0x80001000;
static constexpr const DWORD g_dwSSL_1_0 = 0x0002;
static constexpr const DWORD g_dwSSL_2_0 = 0x0008;
static constexpr const DWORD g_dwSSL_3_0 = 0x0020;
static constexpr const DWORD g_dwTLS_1_0 = 0x0080;
static constexpr const DWORD g_dwTLS_1_1 = 0x0200;
static constexpr const DWORD g_dwTLS_1_2 = 0x0800;
static HANDLE g_hProcHeap;
static wchar_t g_wBuf[MAX_PATH+1];
struct SIpfilter
{
    u_long iIp1;
    u_long iIp2;
};
static SIpfilter *g_pSIpFilter,
*g_pSIpFilterEnd;
static DWORD g_dwHostsNumLines;
static size_t *g_szHostsLengths;
static char *g_cHostsFile;
static wchar_t *g_wHostsFile;
static char **g_cHostsLines;
static wchar_t **g_wHostsLines;
static bool g_bIsHostsBlackList;
static char **g_cLogDomains;
static wchar_t **g_wLogDomains;
static DWORD g_dwLogCurrentA,
g_dwLogCurrentW;

//-------------------------------------------------------------------------------------------------
static inline void FCopyMemory(BYTE *pDst, const BYTE *pSrc, DWORD dwSize)
{
    while (dwSize--)
        *pDst++ = *pSrc++;
}

static inline void FCopyMemoryA(char *pDst, const char *pSrc)
{
    while ((*pDst++ = *pSrc++));
}

static inline void FCopyMemoryW(wchar_t *pDst, const wchar_t *pSrc)
{
    while ((*pDst++ = *pSrc++));
}

static inline bool FCompareMemoryA(const char *pBuf1, const char *pBuf2)
{
    while (*pBuf1 == *pBuf2 && *pBuf2)
        ++pBuf1, ++pBuf2;
    return *pBuf1 == *pBuf2;
}

static inline bool FCompareMemoryW(const wchar_t *pBuf1, const wchar_t *pBuf2)
{
    while (*pBuf1 == *pBuf2 && *pBuf2)
        ++pBuf1, ++pBuf2;
    return *pBuf1 == *pBuf2;
}

static inline bool FIsStartWithW(const wchar_t *pFullStr, const wchar_t *pBeginStr)
{
    while (*pBeginStr)
        if (*pFullStr++ != *pBeginStr++)
            return false;
    return true;
}

static inline size_t FStrLenA(const char *const pKey)
{
    const char *pIt = pKey;
    while (*pIt++);
    return pIt-pKey-1;
}

static inline size_t FStrLenW(const wchar_t *const pKey)
{
    const wchar_t *pIt = pKey;
    while (*pIt++);
    return pIt-pKey-1;
}

static inline char * FStrChrA(char *pSrc, const char cChar)
{
    while (*pSrc && *pSrc != cChar)
        ++pSrc;
    return *pSrc == cChar ? pSrc : nullptr;
}

//-------------------------------------------------------------------------------------------------
EXPORT HRESULT WINAPI SHGetFolderPathWStub(HWND, int, HANDLE, DWORD, LPWSTR pszPath)
{
    FCopyMemoryW(pszPath, g_wBuf);
    return S_OK;
}

//-------------------------------------------------------------------------------------------------
static WINBOOL WINAPI SHGetSpecialFolderPathWStub(HWND, LPWSTR pszPath, int, WINBOOL)
{
    FCopyMemoryW(pszPath, g_wBuf);
    return TRUE;
}

static HRESULT WINAPI SHGetSpecialFolderLocationStub(HWND, int, PIDLIST_ABSOLUTE *ppidl)
{
    return SHILCreateFromPath(g_wBuf, ppidl, nullptr);
}

static DWORD WINAPI GetTempPathWStub(DWORD, LPWSTR lpBuffer)
{
    const wchar_t *pSrc = g_wBuf;
    while ((*lpBuffer++ = *pSrc++));
    lpBuffer[-1] = L'\\';
    *lpBuffer = L'\0';
    return pSrc-g_wBuf;
}

//-------------------------------------------------------------------------------------------------
typedef HINSTANCE (WINAPI *PShellExecuteW)(HWND hWnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
static PShellExecuteW ShellExecuteWReal;
static HINSTANCE WINAPI ShellExecuteWStub(HWND hWnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
    return (lpFile && FIsStartWithW(lpFile, L"skypecheck:")) ?
                reinterpret_cast<HINSTANCE>(ERROR_FILE_NOT_FOUND) :
                ShellExecuteWReal(hWnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
}

//-------------------------------------------------------------------------------------------------
static LONG WINAPI RegOpenKeyStub(HKEY, LPCVOID, PHKEY)
{
    return ERROR_ACCESS_DENIED;
}

static LONG WINAPI RegOpenKeyExAStub(HKEY, LPCSTR, DWORD, REGSAM, PHKEY)
{
    return ERROR_ACCESS_DENIED;
}

static LONG WINAPI RegOpenKeyExWStub(HKEY hKey, LPCWSTR lpSubKey, DWORD, REGSAM, PHKEY phkResult)
{
    if (hKey == HKEY_CURRENT_USER && lpSubKey && FCompareMemoryW(lpSubKey, L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"))
    {
        *phkResult = reinterpret_cast<HKEY>(g_iKeyDummy);
        return ERROR_SUCCESS;
    }
    return ERROR_ACCESS_DENIED;
}

static LONG WINAPI RegCreateKeyStub(HKEY, LPCVOID, PHKEY)
{
    return ERROR_ACCESS_DENIED;
}

static LONG WINAPI RegCreateKeyExStub(HKEY, LPCVOID, DWORD, LPVOID, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD)
{
    return ERROR_ACCESS_DENIED;
}

typedef LONG (WINAPI *PRegQueryValueExW)(HKEY hKey,LPCWSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData);
static PRegQueryValueExW RegQueryValueExWReal;
static LONG WINAPI RegQueryValueExWStub(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    if (hKey == reinterpret_cast<HKEY>(g_iKeyDummy) && lpValueName && FCompareMemoryW(lpValueName, L"SecureProtocols"))
    {
        if (lpType)
            *lpType = REG_DWORD;
        if (lpData)
            *static_cast<DWORD*>(static_cast<void*>(lpData)) = g_dwTLS_1_2;
        if (lpcbData)
            *lpcbData = sizeof(DWORD);
        return ERROR_SUCCESS;
    }
    return RegQueryValueExWReal(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

typedef LONG (WINAPI *PRegCloseKey)(HKEY hKey);
static PRegCloseKey RegCloseKeyReal;
static LONG WINAPI RegCloseKeyStub(HKEY hKey)
{
    return hKey == reinterpret_cast<HKEY>(g_iKeyDummy) ? ERROR_SUCCESS : RegCloseKeyReal(hKey);
}

//-------------------------------------------------------------------------------------------------
typedef HINTERNET (WINAPI *PInternetOpenA)(LPCSTR lpszAgent, DWORD dwAccessType, LPCSTR lpszProxy, LPCSTR lpszProxyBypass, DWORD dwFlags);
static PInternetOpenA InternetOpenAReal;
static HINTERNET WINAPI InternetOpenAStub(LPCSTR lpszAgent, DWORD dwAccessType, LPCSTR lpszProxy, LPCSTR lpszProxyBypass, DWORD dwFlags)
{
    return InternetOpenAReal(lpszAgent, dwAccessType == INTERNET_OPEN_TYPE_PRECONFIG ? INTERNET_OPEN_TYPE_DIRECT : dwAccessType, lpszProxy, lpszProxyBypass, dwFlags);
}

typedef HINTERNET (WINAPI *PInternetOpenW)(LPCWSTR lpszAgent, DWORD dwAccessType, LPCWSTR lpszProxy, LPCWSTR lpszProxyBypass, DWORD dwFlags);
static PInternetOpenW InternetOpenWReal;
static HINTERNET WINAPI InternetOpenWStub(LPCWSTR lpszAgent, DWORD dwAccessType, LPCWSTR lpszProxy, LPCWSTR lpszProxyBypass, DWORD dwFlags)
{
    return InternetOpenWReal(lpszAgent, dwAccessType == INTERNET_OPEN_TYPE_PRECONFIG ? INTERNET_OPEN_TYPE_DIRECT : dwAccessType, lpszProxy, lpszProxyBypass, dwFlags);
}

//-------------------------------------------------------------------------------------------------
typedef HINTERNET (WINAPI *PInternetConnectA)(HINTERNET hInternet, LPCSTR lpszServerName, INTERNET_PORT nServerPort, LPCSTR lpszUserName, LPCSTR lpszPassword, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext);
static PInternetConnectA InternetConnectAReal;
static HINTERNET WINAPI InternetConnectAStub(HINTERNET hInternet, LPCSTR lpszServerName, INTERNET_PORT nServerPort, LPCSTR lpszUserName, LPCSTR lpszPassword, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext)
{
    if (lpszServerName)
    {
        if (g_dwLogCurrentA < g_dwMaxDomains)
        {
            bool bAdd = true;
            for (DWORD i = 0; i < g_dwLogCurrentA; ++i)
                if (FCompareMemoryA(g_cLogDomains[i], lpszServerName))
                {
                    bAdd = false;
                    break;
                }
            if (bAdd && (g_cLogDomains[g_dwLogCurrentA] = static_cast<char*>(HeapAlloc(g_hProcHeap, 0, FStrLenA(lpszServerName)+1))))
            {
                FCopyMemoryA(g_cLogDomains[g_dwLogCurrentA], lpszServerName);
                ++g_dwLogCurrentA;
            }
        }

        if (nServerPort == INTERNET_DEFAULT_HTTPS_PORT)
        {
            for (DWORD i = 0; i < g_dwHostsNumLines; ++i)
                if (g_cHostsLines[i][1] == '.')
                {
                    const size_t szLen = FStrLenA(lpszServerName);
                    if (szLen > g_szHostsLengths[i] && FCompareMemoryA(lpszServerName+(szLen-g_szHostsLengths[i]), g_cHostsLines[i]+1))
                        return (*g_cHostsLines[i] == '+') ?
                                    InternetConnectAReal(hInternet, lpszServerName, nServerPort, lpszUserName, lpszPassword, dwService, dwFlags, dwContext) : nullptr;
                }
                else if (FCompareMemoryA(g_cHostsLines[i]+1, lpszServerName))
                    return (*g_cHostsLines[i] == '+') ?
                                InternetConnectAReal(hInternet, lpszServerName, nServerPort, lpszUserName, lpszPassword, dwService, dwFlags, dwContext) : nullptr;

            if (g_bIsHostsBlackList)
                return InternetConnectAReal(hInternet, lpszServerName, nServerPort, lpszUserName, lpszPassword, dwService, dwFlags, dwContext);
        }
    }
    return nullptr;
}

typedef HINTERNET (WINAPI *PInternetConnectW)(HINTERNET hInternet, LPCWSTR lpszServerName, INTERNET_PORT nServerPort, LPCWSTR lpszUserName, LPCWSTR lpszPassword, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext);
static PInternetConnectW InternetConnectWReal;
static HINTERNET WINAPI InternetConnectWStub(HINTERNET hInternet, LPCWSTR lpszServerName, INTERNET_PORT nServerPort, LPCWSTR lpszUserName, LPCWSTR lpszPassword, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext)
{
    if (lpszServerName)
    {
        if (g_dwLogCurrentW < g_dwMaxDomains)
        {
            bool bAdd = true;
            for (DWORD i = 0; i < g_dwLogCurrentW; ++i)
                if (FCompareMemoryW(g_wLogDomains[i], lpszServerName))
                {
                    bAdd = false;
                    break;
                }
            if (bAdd && (g_wLogDomains[g_dwLogCurrentW] = static_cast<wchar_t*>(HeapAlloc(g_hProcHeap, 0, (FStrLenW(lpszServerName)+1)*sizeof(wchar_t)))))
            {
                FCopyMemoryW(g_wLogDomains[g_dwLogCurrentW], lpszServerName);
                ++g_dwLogCurrentW;
            }
        }

        if (nServerPort == INTERNET_DEFAULT_HTTPS_PORT)
        {
            for (DWORD i = 0; i < g_dwHostsNumLines; ++i)
                if (g_wHostsLines[i][1] == L'.')
                {
                    const size_t szLen = FStrLenW(lpszServerName);
                    if (szLen > g_szHostsLengths[i] && FCompareMemoryW(lpszServerName+(szLen-g_szHostsLengths[i]), g_wHostsLines[i]+1))
                        return (*g_wHostsLines[i] == L'+') ?
                                    InternetConnectWReal(hInternet, lpszServerName, nServerPort, lpszUserName, lpszPassword, dwService, dwFlags, dwContext) : nullptr;
                }
                else if (FCompareMemoryW(g_wHostsLines[i]+1, lpszServerName))
                    return (*g_wHostsLines[i] == L'+') ?
                                InternetConnectWReal(hInternet, lpszServerName, nServerPort, lpszUserName, lpszPassword, dwService, dwFlags, dwContext) : nullptr;

            if (g_bIsHostsBlackList)
                return InternetConnectWReal(hInternet, lpszServerName, nServerPort, lpszUserName, lpszPassword, dwService, dwFlags, dwContext);
        }
    }
    return nullptr;
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

typedef int (WSAAPI *Pconnect)(SOCKET s, const struct sockaddr *name, int namelen);
static Pconnect connectReal;
int WSAAPI connectStub(SOCKET s, const struct sockaddr *name, int namelen)
{
    return FIsBanned(name) ? SOCKET_ERROR : connectReal(s, name, namelen);
}

//-------------------------------------------------------------------------------------------------
static bool FIsArgvOk()
{
    bool bOk = false;
    if (const wchar_t *wCmdLine = GetCommandLineW())
    {
        while (*wCmdLine == L' ' || *wCmdLine == L'\t')
            ++wCmdLine;

        if (*wCmdLine != L'\0')
        {
            const wchar_t *wSrc = wCmdLine;
            int argc = 0, iNumBslash = 0;
            bool bInQuotes = false;

            //count the number of arguments
            while (true)
            {
                if (*wCmdLine == L'\0' || ((*wCmdLine == L' ' || *wCmdLine == L'\t') && !bInQuotes))
                {
                    ++argc;
                    while (*wCmdLine == L' ' || *wCmdLine == L'\t')
                        ++wCmdLine;
                    if (*wCmdLine == L'\0')
                        break;
                    iNumBslash = 0;
                    continue;
                }
                else if (*wCmdLine == L'\\')
                    ++iNumBslash;
                else if (*wCmdLine == L'\"' && !(iNumBslash & 1))
                {
                    bInQuotes = !bInQuotes;
                    iNumBslash = 0;
                }
                else
                    iNumBslash = 0;
                ++wCmdLine;
            }

            if (wchar_t **argv = static_cast<wchar_t**>(HeapAlloc(g_hProcHeap, HEAP_NO_SERIALIZE, argc*sizeof(wchar_t*) + (FStrLenW(wSrc) + 1)*sizeof(wchar_t))))
            {
                wchar_t *wArg = static_cast<wchar_t*>(static_cast<void*>(argv + argc)),
                        *wDest = wArg;
                argc = 0;
                iNumBslash = 0;
                bInQuotes = false;

                //fill the argument array
                while (true)
                {
                    if (*wSrc == L'\0' || ((*wSrc == L' ' || *wSrc == L'\t') && !bInQuotes))
                    {
                        *wDest++ = L'\0';
                        argv[argc++] = wArg;
                        while (*wSrc == L' ' || *wSrc == L'\t')
                            ++wSrc;
                        if (*wSrc == L'\0')
                            break;
                        wArg = wDest;
                        iNumBslash = 0;
                        continue;
                    }
                    else if (*wSrc == L'\\')
                    {
                        *wDest++ = L'\\';
                        ++wSrc;
                        ++iNumBslash;
                    }
                    else if (*wSrc == L'\"')
                    {
                        if (!(iNumBslash & 1))
                        {
                            wDest -= iNumBslash/2;
                            bInQuotes = !bInQuotes;
                        }
                        else
                        {
                            wDest -= (iNumBslash + 1)/2;
                            *wDest++ = L'\"';
                        }
                        ++wSrc;
                        iNumBslash = 0;
                    }
                    else
                    {
                        *wDest++ = *wSrc++;
                        iNumBslash = 0;
                    }
                }

                //check
                for (int i = 1; i < argc; ++i)
                    if (FCompareMemoryW(argv[i], L"/removable"))
                    {
                        for (i = 1; i < argc; ++i)
                            if (FCompareMemoryW(argv[i], L"/secondary"))
                            {
                                for (i = 1; i < argc; ++i)
                                    if (FIsStartWithW(argv[i], L"/datapath:"))
                                    {
                                        bOk = true;
                                        break;
                                    }
                                break;
                            }
                        break;
                    }
                HeapFree(g_hProcHeap, HEAP_NO_SERIALIZE, argv);
            }
        }
    }
    return bOk;
}

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
static bool FPatch(BYTE *pAddress, size_t szOffset, void **const ppOriginal)
{
    const SIZE_T szLen = ppOriginal ? (pAddress -= g_dwPatchSize, g_dwPatchSize+2) : g_dwPatchSize;
    DWORD dwOldProtect;
    if (VirtualProtect(pAddress, szLen, PAGE_EXECUTE_READWRITE, &dwOldProtect))
    {
        szOffset -= reinterpret_cast<size_t>(pAddress) + g_dwPatchSize;
        const BYTE *pByte = static_cast<const BYTE*>(static_cast<const void*>(&szOffset));
        pAddress[0] = 0xE9;        //jump near
        pAddress[1] = pByte[0];
        pAddress[2] = pByte[1];
        pAddress[3] = pByte[2];
        pAddress[4] = pByte[3];
        if (ppOriginal)
        {
            pAddress[5] = 0xEB;        //jump short
            pAddress[6] = 0xF9;        //-7
            *ppOriginal = pAddress+g_dwPatchSize+2;
        }
        DWORD dwTemp;
        if (VirtualProtect(pAddress, szLen, dwOldProtect, &dwTemp))
            return true;
    }
    return false;
}

template <typename T1, typename T2>
static inline bool FCreateHook(T1 *const pTarget, T2 *const pDetour, T1 **const ppOriginal = nullptr)
{return FPatch(reinterpret_cast<BYTE*>(pTarget), reinterpret_cast<size_t>(pDetour), reinterpret_cast<void**>(ppOriginal));}

//-------------------------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID)
{
    static wchar_t *pDelim;
    static BYTE btGetTempPathW[g_dwPatchSize];
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DWORD dwTemp = GetModuleFileNameW(nullptr, g_wBuf, MAX_PATH+1);
        if (dwTemp >= 6 && dwTemp < MAX_PATH)
        {
            pDelim = g_wBuf+dwTemp;
            do
            {
                if (*--pDelim == L'\\')
                    break;
            } while (pDelim > g_wBuf);
            if (pDelim >= g_wBuf+4 && pDelim <= g_wBuf+MAX_PATH-g_dwPathMargin)
            {
                const char *cError = nullptr;
                if ((g_hProcHeap = GetProcessHeap()))
                {
                    if (FIsArgvOk())
                    {
                        LARGE_INTEGER iFileSize;

                        //Ipfilter.cfg-----------------------------------------------------------------
                        FCopyMemoryW(pDelim+1, L"ipfilter.cfg");
                        HANDLE hFile = CreateFileW(g_wBuf, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                        if (hFile != INVALID_HANDLE_VALUE)
                        {
                            //32 = "000.000.000.000-255.255.255.255`"
                            if (GetFileSizeEx(hFile, &iFileSize) && iFileSize.HighPart == 0 && iFileSize.LowPart <= 16*1024*1024 && iFileSize.LowPart%32 == 0)
                            {
                                if (char *const cFile = static_cast<char*>(HeapAlloc(g_hProcHeap, HEAP_NO_SERIALIZE, iFileSize.LowPart)))
                                {
                                    dwTemp = ReadFile(hFile, cFile, iFileSize.LowPart, &dwTemp, nullptr) && dwTemp == iFileSize.LowPart;
                                    CloseHandle(hFile);
                                    if (dwTemp)
                                    {
                                        dwTemp = iFileSize.LowPart/32;
                                        if ((g_pSIpFilter = static_cast<SIpfilter*>(HeapAlloc(g_hProcHeap, HEAP_NO_SERIALIZE, dwTemp*sizeof(SIpfilter)))))
                                        {
                                            g_pSIpFilterEnd = g_pSIpFilter + dwTemp;
                                            const char *cIt = cFile;
                                            for (SIpfilter *pIt = g_pSIpFilter; pIt < g_pSIpFilterEnd; ++pIt, cIt += 32)
                                                if (!FFillEntry(cIt, pIt))
                                                {
                                                    cError = "Invalid record in ipfilter.cfg";
                                                    break;
                                                }
                                        }
                                        else
                                            cError = "Allocation memory error";
                                    }
                                    else
                                        cError = "Reading ipfilter.cfg file failed";
                                    HeapFree(g_hProcHeap, HEAP_NO_SERIALIZE, cFile);
                                }
                                else
                                {
                                    CloseHandle(hFile);
                                    cError = "Allocation memory error";
                                }
                            }
                            else
                            {
                                CloseHandle(hFile);
                                cError = "Invalid ipfilter.cfg file size";
                            }
                        }
                        else
                            cError = "Opening ipfilter.cfg file failed";

                        //Hosts.cfg--------------------------------------------------------------------
                        if (!cError)
                        {
                            FCopyMemoryW(pDelim+1, L"hosts.cfg");
                            hFile = CreateFileW(g_wBuf, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                            if (hFile != INVALID_HANDLE_VALUE)
                            {
                                if (GetFileSizeEx(hFile, &iFileSize) && iFileSize.QuadPart >= 4 && iFileSize.QuadPart <= 128*1024)
                                {
                                    if ((g_cHostsFile = static_cast<char*>(HeapAlloc(g_hProcHeap, HEAP_NO_SERIALIZE, iFileSize.LowPart+1))))
                                    {
                                        dwTemp = ReadFile(hFile, g_cHostsFile, iFileSize.LowPart, &dwTemp, nullptr) && dwTemp == iFileSize.LowPart;
                                        CloseHandle(hFile);
                                        if (dwTemp)
                                        {
                                            if (g_cHostsFile[1] == '\n' && ((g_bIsHostsBlackList = true, *g_cHostsFile == 'B') || (g_bIsHostsBlackList = false, *g_cHostsFile == 'W')))
                                            {
                                                g_cHostsFile[iFileSize.LowPart] = '\0';
                                                char *cIt = g_cHostsFile+2;
                                                g_dwHostsNumLines = 1;
                                                while ((cIt = FStrChrA(cIt+1, '\n')))
                                                {
                                                    *cIt = '\0';
                                                    ++g_dwHostsNumLines;
                                                }

                                                if ((g_cHostsLines = static_cast<char**>(HeapAlloc(g_hProcHeap, HEAP_NO_SERIALIZE, g_dwHostsNumLines*sizeof(char*)))))
                                                {
                                                    if ((g_szHostsLengths = static_cast<size_t*>(HeapAlloc(g_hProcHeap, HEAP_NO_SERIALIZE, g_dwHostsNumLines*sizeof(size_t)))))
                                                    {
                                                        cIt = g_cHostsFile+2;
                                                        char *cIt2;
                                                        for (DWORD i = 0; i < g_dwHostsNumLines; ++i)
                                                        {
                                                            if ((*cIt == '+' || *cIt == '-') && cIt[1] != '\0')
                                                            {
                                                                g_cHostsLines[i] = cIt;
                                                                cIt2 = FStrChrA(cIt, '\0');
                                                                g_szHostsLengths[i] = cIt2-cIt-1;
                                                                cIt = cIt2+1;
                                                            }
                                                            else
                                                            {
                                                                cError = "Invalid record in hosts.cfg";
                                                                break;
                                                            }
                                                        }

                                                        if (!cError)
                                                        {
                                                            ++iFileSize.LowPart;
                                                            if ((g_wHostsFile = static_cast<wchar_t*>(HeapAlloc(g_hProcHeap, HEAP_NO_SERIALIZE, iFileSize.LowPart*sizeof(wchar_t)))))
                                                            {
                                                                if ((g_wHostsLines = static_cast<wchar_t**>(HeapAlloc(g_hProcHeap, HEAP_NO_SERIALIZE, g_dwHostsNumLines*sizeof(wchar_t*)))))
                                                                {
                                                                    cIt = g_cHostsFile;
                                                                    const char *const cEnd = g_cHostsFile+iFileSize.LowPart;
                                                                    wchar_t *wIt = g_wHostsFile;

                                                                    while (cIt < cEnd)
                                                                        *wIt++ = *cIt++;

                                                                    for (DWORD i = 0; i < g_dwHostsNumLines; ++i)
                                                                        g_wHostsLines[i] = g_wHostsFile + (g_cHostsLines[i] - g_cHostsFile);

                                                                    if ((g_wLogDomains = static_cast<wchar_t**>(HeapAlloc(g_hProcHeap, HEAP_NO_SERIALIZE, g_dwMaxDomains*sizeof(wchar_t*)))))
                                                                    {
                                                                        if (!(g_cLogDomains = static_cast<char**>(HeapAlloc(g_hProcHeap, HEAP_NO_SERIALIZE, g_dwMaxDomains*sizeof(char*)))))
                                                                            cError = "Allocation memory error";
                                                                    }
                                                                    else
                                                                        cError = "Allocation memory error";
                                                                }
                                                                else
                                                                    cError = "Allocation memory error";
                                                            }
                                                            else
                                                                cError = "Allocation memory error";
                                                        }
                                                    }
                                                    else
                                                        cError = "Allocation memory error";
                                                }
                                                else
                                                    cError = "Allocation memory error";
                                            }
                                            else
                                                cError = "Invalid hosts.cfg file header";
                                        }
                                        else
                                            cError = "Reading hosts.cfg file failed";
                                    }
                                    else
                                    {
                                        CloseHandle(hFile);
                                        cError = "Allocation memory error";
                                    }
                                }
                                else
                                {
                                    CloseHandle(hFile);
                                    cError = "Invalid hosts.cfg file size";
                                }
                            }
                            else
                                cError = "Opening hosts.cfg file failed";
                        }

                        //Other------------------------------------------------------------------------
                        if (!cError)
                        {
                            pDelim[1] = L'\0';
                            if (SetCurrentDirectoryW(g_wBuf))
                            {
                                *pDelim = L'\0';
                                if (SetEnvironmentVariableW(L"USERPROFILE", g_wBuf))
                                {
                                    if (
                                            FCreateHook(SHGetFolderPathW, SHGetFolderPathWStub) &&
                                            FCreateHook(SHGetSpecialFolderPathW, SHGetSpecialFolderPathWStub) &&
                                            FCreateHook(SHGetSpecialFolderLocation, SHGetSpecialFolderLocationStub) &&
                                            FCreateHook(ShellExecuteW, ShellExecuteWStub, &ShellExecuteWReal) &&
                                            FCreateHook(RegOpenKeyA, RegOpenKeyStub) &&
                                            FCreateHook(RegOpenKeyW, RegOpenKeyStub) &&
                                            FCreateHook(RegOpenKeyExA, RegOpenKeyExAStub) &&
                                            FCreateHook(RegOpenKeyExW, RegOpenKeyExWStub) &&
                                            FCreateHook(RegCreateKeyA, RegCreateKeyStub) &&
                                            FCreateHook(RegCreateKeyW, RegCreateKeyStub) &&
                                            FCreateHook(RegCreateKeyExA, RegCreateKeyExStub) &&
                                            FCreateHook(RegCreateKeyExW, RegCreateKeyExStub) &&
                                            FCreateHook(RegQueryValueExW, RegQueryValueExWStub, &RegQueryValueExWReal) &&
                                            FCreateHook(RegCloseKey, RegCloseKeyStub, &RegCloseKeyReal) &&
                                            FCreateHook(InternetOpenA, InternetOpenAStub, &InternetOpenAReal) &&
                                            FCreateHook(InternetOpenW, InternetOpenWStub, &InternetOpenWReal) &&
                                            FCreateHook(InternetConnectA, InternetConnectAStub, &InternetConnectAReal) &&
                                            FCreateHook(InternetConnectW, InternetConnectWStub, &InternetConnectWReal) &&
                                            ((g_pSIpFilter == g_pSIpFilterEnd) ||
                                             (FCreateHook(sendto, sendtoStub, &sendtoReal) &&
                                              FCreateHook(connect, connectStub, &connectReal))) &&
                                            (FCopyMemory(btGetTempPathW, reinterpret_cast<BYTE*>(GetTempPathW), g_dwPatchSize),
                                             FCreateHook(GetTempPathW, GetTempPathWStub)))        //kernel32.dll (non-hotpatch)
                                    {
                                        if (DisableThreadLibraryCalls(hInstDll))
                                            return TRUE;
                                        cError = "DisableThreadLibraryCalls failed";
                                    }
                                    else
                                        cError = "Setting hooks failed";
                                }
                                else
                                    cError = "SetEnvironmentVariableW failed";
                            }
                            else
                                cError = "SetCurrentDirectoryW failed";
                        }
                    }
                    else
                        cError = "You have to run with \"removable\", \"/secondary\" and \"/datapath:<your_directory>\" arguments";
                }
                else
                    cError = "GetProcessHeap failed";

                if (cError)
                {
                    FCopyMemoryW(pDelim, L"\\log.log");
                    const HANDLE hFile = CreateFileW(g_wBuf, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                    if (hFile != INVALID_HANDLE_VALUE)
                    {
                        WriteFile(hFile, cError, FStrLenA(cError), &dwTemp, nullptr);
                        CloseHandle(hFile);
                    }
                }
            }
        }
    }
    else if (fdwReason == DLL_PROCESS_DETACH && g_pSIpFilter)
    {
        if (g_cHostsFile)
        {
            if (g_cHostsLines)
            {
                if (g_szHostsLengths)
                {
                    if (g_wHostsFile)
                    {
                        if (g_wHostsLines)
                        {
                            if (g_wLogDomains)
                            {
                                if (g_cLogDomains)
                                {
                                    for (DWORD i = 0; i < g_dwLogCurrentW; ++i)
                                    {
                                        const wchar_t *wNew = g_wLogDomains[i];
                                        if (g_dwLogCurrentA < g_dwMaxDomains)
                                        {
                                            char *cIt;
                                            bool bAdd = true;
                                            for (DWORD i = 0; i < g_dwLogCurrentA; ++i)
                                            {
                                                cIt = g_cLogDomains[i];
                                                const wchar_t *wIt = wNew;
                                                while (*cIt == *wIt)
                                                {
                                                    if (*cIt == '\0')
                                                    {
                                                        bAdd = false;
                                                        break;
                                                    }
                                                    ++cIt;
                                                    ++wIt;
                                                }
                                            }
                                            if (bAdd && (g_cLogDomains[g_dwLogCurrentA] = static_cast<char*>(HeapAlloc(g_hProcHeap, HEAP_NO_SERIALIZE, FStrLenW(wNew)+1))))
                                            {
                                                cIt = g_cLogDomains[g_dwLogCurrentA];
                                                while ((*cIt++ = *wNew++));
                                                ++g_dwLogCurrentA;
                                            }
                                        }
                                        HeapFree(g_hProcHeap, HEAP_NO_SERIALIZE, g_wLogDomains[i]);
                                    }

                                    if (g_dwLogCurrentA)
                                    {
                                        FCopyMemoryW(pDelim, L"\\log.log");
                                        const HANDLE hFile = CreateFileW(g_wBuf, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                                        if (hFile != INVALID_HANDLE_VALUE)
                                        {
                                            DWORD dwBytes;
                                            for (DWORD i = 0; i < g_dwLogCurrentA; ++i)
                                            {
                                                const size_t szLen = FStrLenA(g_cLogDomains[i]);
                                                g_cLogDomains[i][szLen] = '\n';
                                                WriteFile(hFile, g_cLogDomains[i], szLen+1, &dwBytes, nullptr);
                                            }
                                            CloseHandle(hFile);
                                        }
                                    }

                                    for (DWORD i = 0; i < g_dwLogCurrentA; ++i)
                                        HeapFree(g_hProcHeap, HEAP_NO_SERIALIZE, g_cLogDomains[i]);
                                    HeapFree(g_hProcHeap, HEAP_NO_SERIALIZE, g_cLogDomains);

                                    if (*btGetTempPathW)
                                    {
                                        BYTE *const pAddress = reinterpret_cast<BYTE*>(GetTempPathW);
                                        DWORD dwOldProtect;
                                        if (VirtualProtect(pAddress, g_dwPatchSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                                        {
                                            FCopyMemory(pAddress, btGetTempPathW, g_dwPatchSize);
                                            DWORD dwTemp;
                                            VirtualProtect(pAddress, g_dwPatchSize, dwOldProtect, &dwTemp);
                                        }
                                    }
                                }
                                HeapFree(g_hProcHeap, HEAP_NO_SERIALIZE, g_wLogDomains);
                            }
                            HeapFree(g_hProcHeap, HEAP_NO_SERIALIZE, g_wHostsLines);
                        }
                        HeapFree(g_hProcHeap, HEAP_NO_SERIALIZE, g_wHostsFile);
                    }
                    HeapFree(g_hProcHeap, HEAP_NO_SERIALIZE, g_szHostsLengths);
                }
                HeapFree(g_hProcHeap, HEAP_NO_SERIALIZE, g_cHostsLines);
            }
            HeapFree(g_hProcHeap, HEAP_NO_SERIALIZE, g_cHostsFile);
        }
        HeapFree(g_hProcHeap, HEAP_NO_SERIALIZE, g_pSIpFilter);
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
