//qBittorrentPortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#include <ws2tcpip.h>
#include <shlobj.h>
#ifdef _WIN64
#include "MinHook/MinHook.h"
#else
#include <powrprof.h>
#endif

#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 77;        //"\qBittorrent\BT_backup\0123456789abcdefghijklmnopqrstuvwxyz????.fastresume.0`"
static constexpr const DWORD g_dwPatchSize = 1+sizeof(size_t);
static wchar_t g_wBuf[MAX_PATH+1];
static SIZE_T g_szPathBytes;

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

static inline void FCopyMemory(BYTE *pDst, const BYTE *pSrc)
{
    DWORD dwSize = g_dwPatchSize;
    while (dwSize--)
        *pDst++ = *pSrc++;
}

#ifdef _WIN64
//-------------------------------------------------------------------------------------------------
EXPORT LPWSTR * WINAPI CommandLineToArgvWStub(LPCWSTR lpCmdLine, int *pNumArgs)
{
    return CommandLineToArgvW(lpCmdLine, pNumArgs);
}

EXPORT WINBOOL WINAPI SHGetSpecialFolderPathWStub(HWND, LPWSTR pszPath, int, WINBOOL)
{
    const wchar_t *pSrc = g_wBuf;
    while ((*pszPath++ = *pSrc++));
    return TRUE;
}

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
#else
//-------------------------------------------------------------------------------------------------
EXPORT BOOLEAN WINAPI SetSuspendStateStub(BOOLEAN Hibernate, BOOLEAN ForceCritical, BOOLEAN DisableWakeEvent)
{
    return SetSuspendState(Hibernate, ForceCritical, DisableWakeEvent);
}

static WINBOOL WINAPI SHGetSpecialFolderPathWStub(HWND, LPWSTR pszPath, int, WINBOOL)
{
    const wchar_t *pSrc = g_wBuf;
    while ((*pszPath++ = *pSrc++));
    return TRUE;
}

static HRESULT WINAPI SHGetKnownFolderPathStub(REFKNOWNFOLDERID, DWORD, HANDLE, PWSTR *ppszPath)
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
#endif

static DWORD WINAPI GetTempPathWStub(DWORD, LPWSTR lpBuffer)
{
    const wchar_t *pSrc = g_wBuf;
    while ((*lpBuffer++ = *pSrc++));
    lpBuffer[-1] = L'\\';
    *lpBuffer = L'\0';
    return pSrc-g_wBuf;
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

typedef int WSAAPI (*PWSASendTo)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, const struct sockaddr *lpTo, int iTolen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
static PWSASendTo WSASendToReal;
static int WSAAPI WSASendToStub(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, const struct sockaddr *lpTo, int iTolen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    return FIsBanned(lpTo) ? WSAEHOSTUNREACH : WSASendToReal(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpTo, iTolen, lpOverlapped, lpCompletionRoutine);
}

typedef int WSAAPI (*PWSAConnect)(SOCKET s, const struct sockaddr *name, int namelen, LPWSABUF lpCallerData, LPWSABUF lpCalleeData, LPQOS lpSQOS, LPQOS lpGQOS);
static PWSAConnect WSAConnectReal;
static int WSAAPI WSAConnectStub(SOCKET s, const struct sockaddr *name, int namelen, LPWSABUF lpCallerData, LPWSABUF lpCalleeData, LPQOS lpSQOS, LPQOS lpGQOS)
{
    return FIsBanned(name) ? WSAEHOSTUNREACH : WSAConnectReal(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS);
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

#ifdef _WIN64
//-------------------------------------------------------------------------------------------------
template <typename T>
static inline bool FCreateHook(T *const pTarget, T *const pDetour, T **const ppOriginal = nullptr)
{return MH_CreateHook(reinterpret_cast<LPVOID>(pTarget), reinterpret_cast<LPVOID>(pDetour), reinterpret_cast<LPVOID*>(ppOriginal)) == MH_OK;}
#else
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
#endif

//-------------------------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID)
{
#ifndef _WIN64
    static BYTE btGetTempPathW[g_dwPatchSize];
#endif
    if (fdwReason == DLL_PROCESS_ATTACH)
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
#ifndef _WIN64
                                FCopyMemory(btGetTempPathW, reinterpret_cast<BYTE*>(GetTempPathW));
#endif
                                if (HeapFree(hProcHeap, HEAP_NO_SERIALIZE, cFile) && bOk &&
                                        (pDelim[1] = L'\0', SetCurrentDirectoryW(g_wBuf)))
#ifdef _WIN64
                                    if (MH_Initialize() == MH_OK)
#endif
                                        if (
                                                FCreateHook(SHGetSpecialFolderPathW, SHGetSpecialFolderPathWStub) &&
                                                FCreateHook(SHGetKnownFolderPath, SHGetKnownFolderPathStub) &&
                                                FCreateHook(GetTempPathW, GetTempPathWStub) &&
                                                FCreateHook(RegCreateKeyExW, RegCreateKeyExWStub) &&
                                                (g_pSIpFilter == g_pSIpFilterEnd ||
                                                 (FCreateHook(WSASendTo, WSASendToStub, &WSASendToReal) &&
                                                  FCreateHook(WSAConnect, WSAConnectStub, &WSAConnectReal))))
#ifdef _WIN64
                                            if (MH_EnableHook(MH_ALL_HOOKS) == MH_OK)
#endif
                                                if (DisableThreadLibraryCalls(hInstDll))
                                                {
                                                    g_szPathBytes = (pDelim-g_wBuf+1)*sizeof(wchar_t);
                                                    *pDelim = L'\0';
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
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        if (g_pSIpFilter)
            if (const HANDLE hProcHeap = GetProcessHeap())
                HeapFree(hProcHeap, HEAP_NO_SERIALIZE, g_pSIpFilter);
#ifdef _WIN64
        MH_Uninitialize();
#else
        if (*btGetTempPathW)
        {
            BYTE *const pAddress = reinterpret_cast<BYTE*>(GetTempPathW);
            DWORD dwOldProtect;
            if (VirtualProtect(pAddress, g_dwPatchSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
            {
                pAddress[0] = btGetTempPathW[0];
                pAddress[1] = btGetTempPathW[1];
                pAddress[2] = btGetTempPathW[2];
                pAddress[3] = btGetTempPathW[3];
                DWORD dwTemp;
                VirtualProtect(pAddress, g_dwPatchSize, dwOldProtect, &dwTemp);
            }
        }
#endif
    }
    return FALSE;
}
