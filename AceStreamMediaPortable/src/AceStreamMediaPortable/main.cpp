//AceStreamMediaPortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#include <ws2tcpip.h>

#ifndef _WIN64
#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 101;        //"\AppData\Roaming\.ACEStream\collected_torrent_files\0123456789abcdefghijklmnopqrstuvwxyz????.torrent`"

//-------------------------------------------------------------------------------------------------
static inline bool FCompareMemoryA(const char *pBuf1, const char *pBuf2)
{
    while (*pBuf1 == *pBuf2 && *pBuf2)
        ++pBuf1, ++pBuf2;
    return *pBuf1 == *pBuf2;
}

//-------------------------------------------------------------------------------------------------
EXPORT HWND WINAPI GetFocusStub()
{
    return GetFocus();
}

EXPORT int WINAPI MessageBoxAStub(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
    return MessageBoxA(hWnd, lpText, lpCaption, uType);
}

//-------------------------------------------------------------------------------------------------
static LONG WINAPI RegOpenKeyExStub(HKEY, LPCVOID, DWORD, REGSAM, PHKEY)
{
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

//-------------------------------------------------------------------------------------------------
typedef int (WSAAPI *Pgetaddrinfo)(const char *nodename, const char *servname, const struct addrinfo *hints, struct addrinfo **res);
static Pgetaddrinfo getaddrinfoReal;
static int WSAAPI getaddrinfoStub(const char *nodename, const char *servname, const struct addrinfo *hints, struct addrinfo **res)
{
    return (nodename && (FCompareMemoryA(nodename, "www.google-analytics.com") || FCompareMemoryA(nodename, "google.com"))) ?
                WSAHOST_NOT_FOUND : getaddrinfoReal(nodename, servname, hints, res);
}

//-------------------------------------------------------------------------------------------------
static bool FIsBanned(const void *const pSockAddr)
{
    const u_char *const pOctet = &static_cast<const sockaddr_in*>(pSockAddr)->sin_addr.S_un.S_un_b.s_b1;
    return (*pOctet == 0 ||
            *pOctet == 10 ||
            (*pOctet == 100 && (pOctet[1] >= 64 && pOctet[1] <= 127)) ||
            (*pOctet == 127 && !(pOctet[1] == 0 && pOctet[2] == 0 && pOctet[3] == 1)) ||
            (*pOctet == 169 && pOctet[1] == 254) ||
            (*pOctet == 172 && (pOctet[1] >= 16 && pOctet[1] <= 31)) ||
            (*pOctet == 192 &&
            ((pOctet[1] == 0 && pOctet[2] == 0) ||
            (pOctet[1] == 0 && pOctet[2] == 2) ||
            (pOctet[1] == 88 && pOctet[2] == 99) ||
            pOctet[1] == 168)) ||
            (*pOctet == 198 &&
            ((pOctet[1] >= 18 && pOctet[1] <= 19) ||
            (pOctet[1] == 51 && pOctet[2] == 100))) ||
            (*pOctet == 203 && pOctet[1] == 0 && pOctet[2] == 113) ||
            *pOctet >= 224);
}

//-------------------------------------------------------------------------------------------------
typedef int (WSAAPI *Psendto)(SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen);
static Psendto sendtoReal;
static int WSAAPI sendtoStub(SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen)
{
    return FIsBanned(to) ? WSAEHOSTUNREACH : sendtoReal(s, buf, len, flags, to, tolen);
}

//-------------------------------------------------------------------------------------------------
typedef int (WSAAPI *Pconnect)(SOCKET s, const struct sockaddr *name, int namelen);
static Pconnect connectReal;
static int WSAAPI connectStub(SOCKET s, const struct sockaddr *name, int namelen)
{
    return FIsBanned(name) ? SOCKET_ERROR : connectReal(s, name, namelen);
}

//-------------------------------------------------------------------------------------------------
typedef WINBOOL (WINAPI *PCreateProcessA)(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, WINBOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
static PCreateProcessA CreateProcessAReal;
static WINBOOL WINAPI CreateProcessAStub(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, WINBOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
    if (lpCommandLine)
    {
        const char *cArg = lpCommandLine;
        while (*cArg)
            ++cArg;
        cArg -= 12;
        if ((cArg > lpCommandLine && FCompareMemoryA(cArg, " --js-player")) || ((cArg -= 7, cArg > lpCommandLine) && FCompareMemoryA(cArg, " --js-player --stop")))
            return FALSE;
    }
    return CreateProcessAReal(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}

//-------------------------------------------------------------------------------------------------
static bool FPatch(BYTE *pAddress, size_t szOffset, void **const ppOriginal)
{
    constexpr const DWORD dwPatchSize = 1+sizeof(size_t);
    const SIZE_T szLen = ppOriginal ? (pAddress -= dwPatchSize, dwPatchSize+2) : dwPatchSize;
    DWORD dwOldProtect;
    if (VirtualProtect(pAddress, szLen, PAGE_EXECUTE_READWRITE, &dwOldProtect))
    {
        szOffset -= reinterpret_cast<size_t>(pAddress) + dwPatchSize;
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
            *ppOriginal = pAddress+dwPatchSize+2;
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
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        wchar_t wBuf[MAX_PATH+1];
        const DWORD dwLen = GetModuleFileNameW(nullptr, wBuf, MAX_PATH+1);
        if (dwLen >= 6 && dwLen < MAX_PATH)
        {
            wchar_t *pDelim = wBuf+dwLen;
            do
            {
                if (*--pDelim == L'\\')
                    break;
            } while (pDelim > wBuf);
            if (pDelim >= wBuf+4 && pDelim <= wBuf + MAX_PATH-g_dwPathMargin &&
                    (*pDelim = L'\0', SetEnvironmentVariableW(L"USERPROFILE", wBuf)) &&
                    FCreateHook(getaddrinfo, getaddrinfoStub, &getaddrinfoReal) &&
                    FCreateHook(sendto, sendtoStub, &sendtoReal) &&
                    FCreateHook(connect, connectStub, &connectReal) &&
                    FCreateHook(RegOpenKeyExA, RegOpenKeyExStub) &&
                    FCreateHook(RegOpenKeyExW, RegOpenKeyExStub) &&
                    FCreateHook(RegCreateKeyA, RegCreateKeyStub) &&
                    FCreateHook(RegCreateKeyW, RegCreateKeyStub) &&
                    FCreateHook(RegCreateKeyExA, RegCreateKeyExStub) &&
                    FCreateHook(RegCreateKeyExW, RegCreateKeyExStub) &&
                    FCreateHook(CreateProcessA, CreateProcessAStub, &CreateProcessAReal) &&
                    DisableThreadLibraryCalls(hInstDll))
                return TRUE;
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
