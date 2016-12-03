//AdobeFlashPlayerPluginPortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#define WIN32_LEAN_AND_MEAN
#include <shlobj.h>
#include <tlhelp32.h>
#ifdef _WIN64
#include "MinHook/MinHook.h"
#endif

#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 84;        //"\Macromedia\Flash Player\macromedia.com\support\flashplayer\sys\#local\settings.sol`"
static wchar_t g_wBuf[MAX_PATH-g_dwPathMargin+1];

//-------------------------------------------------------------------------------------------------
static inline wchar_t * FStrRChrWDelim(wchar_t *pSrc)
{
    wchar_t *pRes = nullptr;
    while (*pSrc)
    {
        if (*pSrc == L'\\')
            pRes = pSrc;
        ++pSrc;
    }
    return pRes;
}

//-------------------------------------------------------------------------------------------------
static HRESULT WINAPI SHGetFolderPathWStub(HWND, int, HANDLE, DWORD, LPWSTR pszPath)
{
    const wchar_t *pSrc = g_wBuf;
    while ((*pszPath++ = *pSrc++));
    return S_OK;
}

static UINT WINAPI GetSystemWow64DirectoryWStub(LPWSTR lpBuffer, UINT)
{
    const wchar_t *pSrc = g_wBuf;
    while ((*lpBuffer++ = *pSrc++));
    return pSrc-g_wBuf-1;
}

//-------------------------------------------------------------------------------------------------
EXPORT WINBOOL WINAPI CryptAcquireContextAStub(HCRYPTPROV *phProv, LPCSTR szContainer, LPCSTR szProvider, DWORD dwProvType, DWORD dwFlags)
{return CryptAcquireContextA(phProv, szContainer, szProvider, dwProvType, dwFlags);}
EXPORT WINBOOL WINAPI CryptAcquireContextWStub(HCRYPTPROV *phProv, LPCWSTR szContainer, LPCWSTR szProvider, DWORD dwProvType, DWORD dwFlags)
{return CryptAcquireContextW(phProv, szContainer, szProvider, dwProvType, dwFlags);}
EXPORT WINBOOL WINAPI CryptCreateHashStub(HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey, DWORD dwFlags, HCRYPTHASH *phHash)
{return CryptCreateHash(hProv, Algid, hKey, dwFlags, phHash);}
EXPORT WINBOOL WINAPI CryptDecryptStub(HCRYPTKEY hKey, HCRYPTHASH hHash, WINBOOL Final, DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{return CryptDecrypt(hKey, hHash, Final, dwFlags, pbData, pdwDataLen);}
EXPORT WINBOOL WINAPI CryptDestroyHashStub(HCRYPTHASH hHash)
{return CryptDestroyHash(hHash);}
EXPORT WINBOOL WINAPI CryptDestroyKeyStub(HCRYPTKEY hKey)
{return CryptDestroyKey(hKey);}
EXPORT WINBOOL WINAPI CryptEncryptStub(HCRYPTKEY hKey, HCRYPTHASH hHash, WINBOOL Final, DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen, DWORD dwBufLen)
{return CryptEncrypt(hKey, hHash, Final, dwFlags, pbData, pdwDataLen, dwBufLen);}
EXPORT WINBOOL WINAPI CryptExportKeyStub(HCRYPTKEY hKey, HCRYPTKEY hExpKey, DWORD dwBlobType, DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{return CryptExportKey(hKey, hExpKey, dwBlobType, dwFlags, pbData, pdwDataLen);}
EXPORT WINBOOL WINAPI CryptGenKeyStub(HCRYPTPROV hProv, ALG_ID Algid, DWORD dwFlags, HCRYPTKEY *phKey)
{return CryptGenKey(hProv, Algid, dwFlags, phKey);}
EXPORT WINBOOL WINAPI CryptGenRandomStub(HCRYPTPROV hProv, DWORD dwLen, BYTE *pbBuffer)
{return CryptGenRandom(hProv, dwLen, pbBuffer);}
EXPORT WINBOOL WINAPI CryptGetHashParamStub(HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData, DWORD *pdwDataLen, DWORD dwFlags)
{return CryptGetHashParam(hHash, dwParam, pbData, pdwDataLen, dwFlags);}
EXPORT WINBOOL WINAPI CryptHashDataStub(HCRYPTHASH hHash, CONST BYTE *pbData, DWORD dwDataLen, DWORD dwFlags)
{return CryptHashData(hHash, pbData, dwDataLen, dwFlags);}
EXPORT WINBOOL WINAPI CryptImportKeyStub(HCRYPTPROV hProv, CONST BYTE *pbData, DWORD dwDataLen, HCRYPTKEY hPubKey, DWORD dwFlags, HCRYPTKEY *phKey)
{return CryptImportKey(hProv, pbData, dwDataLen, hPubKey, dwFlags, phKey);}
EXPORT WINBOOL WINAPI CryptReleaseContextStub(HCRYPTPROV hProv, DWORD dwFlags)
{return CryptReleaseContext(hProv, dwFlags);}
EXPORT WINBOOL WINAPI CryptSetKeyParamStub(HCRYPTKEY hKey, DWORD dwParam, CONST BYTE *pbData, DWORD dwFlags)
{return CryptSetKeyParam(hKey, dwParam, pbData, dwFlags);}
EXPORT WINBOOL WINAPI DeregisterEventSourceStub(HANDLE hEventLog)
{return DeregisterEventSource(hEventLog);}
EXPORT PDWORD WINAPI GetSidSubAuthorityStub(PSID pSid, DWORD nSubAuthority)
{return GetSidSubAuthority(pSid, nSubAuthority);}
EXPORT PUCHAR WINAPI GetSidSubAuthorityCountStub(PSID pSid)
{return GetSidSubAuthorityCount(pSid);}
EXPORT WINBOOL WINAPI GetTokenInformationStub(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, LPVOID TokenInformation, DWORD TokenInformationLength, PDWORD ReturnLength)
{return GetTokenInformation(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength, ReturnLength);}
EXPORT WINBOOL WINAPI IsValidSidStub(PSID pSid)
{return IsValidSid(pSid);}
EXPORT WINBOOL WINAPI OpenProcessTokenStub(HANDLE ProcessHandle, DWORD DesiredAccess, PHANDLE TokenHandle)
{return OpenProcessToken(ProcessHandle, DesiredAccess, TokenHandle);}
EXPORT LONG WINAPI RegCloseKeyStub(HKEY hKey)
{return RegCloseKey(hKey);}
EXPORT LONG WINAPI RegCreateKeyAStub(HKEY, LPCSTR, PHKEY)
{return ERROR_ACCESS_DENIED;}
EXPORT LONG WINAPI RegCreateKeyExAStub(HKEY, LPCSTR, DWORD, LPSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD)
{return ERROR_ACCESS_DENIED;}
EXPORT LONG WINAPI RegDeleteValueAStub(HKEY, LPCSTR)
{return ERROR_ACCESS_DENIED;}
EXPORT HANDLE WINAPI RegisterEventSourceWStub(LPCWSTR lpUNCServerName, LPCWSTR lpSourceName)
{return RegisterEventSourceW(lpUNCServerName, lpSourceName);}
EXPORT LONG WINAPI RegOpenKeyAStub(HKEY, LPCSTR, PHKEY)
{return ERROR_ACCESS_DENIED;}
EXPORT LONG WINAPI RegOpenKeyExStub(HKEY, LPVOID, DWORD, REGSAM, PHKEY)
{return ERROR_ACCESS_DENIED;}
EXPORT LONG WINAPI RegQueryValueExStub(HKEY, LPVOID, LPDWORD, LPDWORD, LPBYTE, LPDWORD)
{return ERROR_ACCESS_DENIED;}
EXPORT LONG WINAPI RegSetValueExAStub(HKEY, LPCSTR, DWORD, DWORD, CONST BYTE*, DWORD)
{return ERROR_ACCESS_DENIED;}
EXPORT WINBOOL WINAPI ReportEventWStub(HANDLE hEventLog, WORD wType, WORD wCategory, DWORD dwEventID, PSID lpUserSid, WORD wNumStrings, DWORD dwDataSize, LPCWSTR *lpStrings, LPVOID lpRawData)
{return ReportEventW(hEventLog, wType, wCategory, dwEventID, lpUserSid, wNumStrings, dwDataSize, lpStrings, lpRawData);}

#ifdef _WIN64
//-------------------------------------------------------------------------------------------------
template <typename T>
static inline bool FCreateHook(T *const pTarget, T *const pDetour)
{return MH_CreateHook(reinterpret_cast<LPVOID>(pTarget), reinterpret_cast<LPVOID>(pDetour), nullptr) == MH_OK;}
#endif

//-------------------------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        const DWORD dwPid = GetCurrentProcessId();
        if (dwPid != ASFW_ANY)
        {
            const HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPid);
            if (hSnapshot != INVALID_HANDLE_VALUE)
            {
                MODULEENTRY32W moduleEntry32;
                moduleEntry32.dwSize = sizeof(MODULEENTRY32W);
                if (Module32FirstW(hSnapshot, &moduleEntry32))
                {
                    do
                    {
                        if (moduleEntry32.hModule == hInstDll)
                        {
                            if (moduleEntry32.szExePath)
                                if (wchar_t *const pDelim = FStrRChrWDelim(moduleEntry32.szExePath))
                                    if (pDelim >= moduleEntry32.szExePath+4 && pDelim <= moduleEntry32.szExePath+MAX_PATH-g_dwPathMargin)
                                    {
                                        wchar_t *pDst = g_wBuf;
                                        const wchar_t *pSrc = moduleEntry32.szExePath;
                                        *pDelim = L'\0';
                                        while ((*pDst++ = *pSrc++));
                                        *pDelim = L'\\';
                                    }
                            break;
                        }
                    } while (Module32NextW(hSnapshot, &moduleEntry32));
                }
                CloseHandle(hSnapshot);

                if (*g_wBuf)
                {
#ifdef _WIN64
                    if (MH_Initialize() == MH_OK &&
                            FCreateHook(SHGetFolderPathW, SHGetFolderPathWStub) &&
                            FCreateHook(GetSystemWow64DirectoryW, GetSystemWow64DirectoryWStub) &&
                            MH_EnableHook(MH_ALL_HOOKS) == MH_OK &&
                            DisableThreadLibraryCalls(hInstDll))
                        return TRUE;
#else
                    constexpr const DWORD dwPatchSize = 1+sizeof(size_t);
                    BYTE *pAddress = reinterpret_cast<BYTE*>(SHGetFolderPathW);
                    DWORD dwOldProtect;
                    if (VirtualProtect(pAddress, dwPatchSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                    {
                        size_t szOffset = reinterpret_cast<size_t>(SHGetFolderPathWStub) - (reinterpret_cast<size_t>(pAddress) + dwPatchSize);
                        const BYTE *pByte = static_cast<const BYTE*>(static_cast<const void*>(&szOffset));
                        pAddress[0] = 0xE9;        //jump near
                        pAddress[1] = pByte[0];
                        pAddress[2] = pByte[1];
                        pAddress[3] = pByte[2];
                        pAddress[4] = pByte[3];
                        DWORD dwTemp;
                        if (VirtualProtect(pAddress, dwPatchSize, dwOldProtect, &dwTemp))
                        {
                            pAddress = reinterpret_cast<BYTE*>(GetSystemWow64DirectoryW);
                            if (VirtualProtect(pAddress, dwPatchSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                            {
                                szOffset = reinterpret_cast<size_t>(GetSystemWow64DirectoryWStub) - (reinterpret_cast<size_t>(pAddress) + dwPatchSize);
                                pByte = static_cast<const BYTE*>(static_cast<const void*>(&szOffset));
                                pAddress[0] = 0xE9;        //jump near
                                pAddress[1] = pByte[0];
                                pAddress[2] = pByte[1];
                                pAddress[3] = pByte[2];
                                pAddress[4] = pByte[3];
                                if (VirtualProtect(pAddress, dwPatchSize, dwOldProtect, &dwTemp) & DisableThreadLibraryCalls(hInstDll))
                                    return TRUE;
                            }
                        }
                    }
#endif
                }
            }
        }
    }
#ifdef _WIN64
    else if (fdwReason == DLL_PROCESS_DETACH)
        MH_Uninitialize();
    //x32: restore kernel32!GetSystemWow64DirectoryW supposedly don't needed
#endif
    return FALSE;
}
