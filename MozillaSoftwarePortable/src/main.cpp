//MozillaFirefoxAndEtcPortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#define WIN32_LEAN_AND_MEAN
#include <shlobj.h>
#ifdef _WIN64
#include "MinHook/MinHook.h"
#endif

#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 32;        //"\0\SiteSecurityServiceState.txt`"
static constexpr const LONG g_iKeyDummy = 0x80001000;
static wchar_t g_wBuf[MAX_PATH+1];
static SIZE_T g_szPathBytes;

//-------------------------------------------------------------------------------------------------
static inline bool FCompareMemoryW(const wchar_t *pBuf1, const wchar_t *pBuf2)
{
    while (*pBuf1 == *pBuf2 && *pBuf2)
        ++pBuf1, ++pBuf2;
    return *pBuf1 == *pBuf2;
}

//-------------------------------------------------------------------------------------------------
EXPORT WINBOOL WINAPI GradientFillStub(HDC, PTRIVERTEX, ULONG, PVOID, ULONG, ULONG)
{
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
static HRESULT WINAPI SHGetSpecialFolderLocationStub(HWND, int, PIDLIST_ABSOLUTE *ppidl)
{
    return SHILCreateFromPath(g_wBuf, ppidl, nullptr);
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

//-------------------------------------------------------------------------------------------------
static LONG WINAPI RegOpenKeyExAStub(HKEY, LPCSTR, DWORD, REGSAM, PHKEY)
{
    return ERROR_ACCESS_DENIED;
}

static LONG WINAPI RegOpenKeyExWStub(HKEY hKey, LPCWSTR lpSubKey, DWORD, REGSAM, PHKEY phkResult)
{
    if (hKey == HKEY_CURRENT_USER && lpSubKey &&
            (FCompareMemoryW(lpSubKey, L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Attachments") ||
             FCompareMemoryW(lpSubKey, L"Software\\Thunderbird\\Crash Reporter")))
    {
        *phkResult = reinterpret_cast<HKEY>(g_iKeyDummy);
        return ERROR_SUCCESS;
    }
    return ERROR_ACCESS_DENIED;
}

typedef LONG (WINAPI *PRegQueryValueExW)(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
static PRegQueryValueExW RegQueryValueExWReal;
static LONG WINAPI RegQueryValueExWStub(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    if (hKey == reinterpret_cast<HKEY>(g_iKeyDummy))
    {
        if (lpValueName && lpData)
        {
            DWORD dwData = 1;
            if (FCompareMemoryW(lpValueName, L"SaveZoneInformation") ||
                    (dwData = 0, FCompareMemoryW(lpValueName, L"SubmitCrashReport")))
            {
                if (lpType)
                    *lpType = REG_DWORD;
                *static_cast<DWORD*>(static_cast<void*>(lpData)) = dwData;
                if (lpcbData)
                    *lpcbData = sizeof(DWORD);
                return ERROR_SUCCESS;
            }
        }
        return ERROR_ACCESS_DENIED;
    }
    return RegQueryValueExWReal(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

static LONG WINAPI RegCreateKeyExStub(HKEY, LPCVOID, DWORD, LPVOID, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD)
{
    return ERROR_ACCESS_DENIED;
}

typedef LONG (WINAPI *PRegCloseKey)(HKEY hKey);
static PRegCloseKey RegCloseKeyReal;
static LONG WINAPI RegCloseKeyStub(HKEY hKey)
{
    return hKey == reinterpret_cast<HKEY>(g_iKeyDummy) ? ERROR_SUCCESS : RegCloseKeyReal(hKey);
}

#ifdef _WIN64
//-------------------------------------------------------------------------------------------------
template <typename T1, typename T2>
static inline bool FCreateHook(T1 *const pTarget, T2 *const pDetour, T1 **const ppOriginal = nullptr)
{return MH_CreateHook(reinterpret_cast<LPVOID>(pTarget), reinterpret_cast<LPVOID>(pDetour), reinterpret_cast<LPVOID*>(ppOriginal)) == MH_OK;}
#else
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
#endif

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
            if (pDelim >= g_wBuf+4 && pDelim <= g_wBuf+MAX_PATH-g_dwPathMargin)
#ifdef _WIN64
                if (MH_Initialize() == MH_OK)
#endif
                    if (
                            FCreateHook(SHGetSpecialFolderLocation, SHGetSpecialFolderLocationStub) &&
                            FCreateHook(SHGetSpecialFolderPathW, SHGetSpecialFolderPathWStub) &&
                            FCreateHook(SHGetKnownFolderPath, SHGetKnownFolderPathStub) &&
                            FCreateHook(RegOpenKeyExA, RegOpenKeyExAStub) &&
                            FCreateHook(RegOpenKeyExW, RegOpenKeyExWStub) &&
                            FCreateHook(RegCreateKeyExA, RegCreateKeyExStub) &&
                            FCreateHook(RegCreateKeyExW, RegCreateKeyExStub) &&
                            FCreateHook(RegQueryValueExW, RegQueryValueExWStub, &RegQueryValueExWReal) &&
                            FCreateHook(RegCloseKey, RegCloseKeyStub, &RegCloseKeyReal))
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
    }
#ifdef _WIN64
    else if (fdwReason == DLL_PROCESS_DETACH)
        MH_Uninitialize();
#endif
    return FALSE;
}
