//AdobeFlashPlayerStandalonePortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#define WIN32_LEAN_AND_MEAN
#include <shlobj.h>

#ifndef _WIN64
#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 84;        //"\Macromedia\Flash Player\macromedia.com\support\flashplayer\sys\#local\settings.sol`"
static wchar_t g_wBuf[MAX_PATH+1];

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
EXPORT LONG WINAPI RegCloseKeyStub(HKEY hKey)
{return RegCloseKey(hKey);}
EXPORT LONG WINAPI RegCreateKeyExStub(HKEY, LPCVOID, DWORD, LPSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD)
{return ERROR_ACCESS_DENIED;}
EXPORT HANDLE WINAPI RegisterEventSourceAStub(LPCSTR lpUNCServerName, LPCSTR lpSourceName)
{return RegisterEventSourceA(lpUNCServerName, lpSourceName);}
EXPORT LONG WINAPI RegOpenKeyAStub(HKEY, LPCSTR, PHKEY)
{return ERROR_ACCESS_DENIED;}
EXPORT LONG WINAPI RegOpenKeyExStub(HKEY, LPCVOID, DWORD, REGSAM, PHKEY)
{return ERROR_ACCESS_DENIED;}
EXPORT LONG WINAPI RegQueryValueExStub(HKEY, LPCVOID, LPDWORD, LPDWORD, LPBYTE, LPDWORD)
{return ERROR_ACCESS_DENIED;}
EXPORT LONG WINAPI RegSetValueExStub(HKEY, LPCVOID, DWORD, DWORD, CONST BYTE*, DWORD)
{return ERROR_ACCESS_DENIED;}
EXPORT WINBOOL WINAPI ReportEventAStub(HANDLE hEventLog, WORD wType, WORD wCategory, DWORD dwEventID, PSID lpUserSid, WORD wNumStrings, DWORD dwDataSize, LPCSTR *lpStrings, LPVOID lpRawData)
{return ReportEventA(hEventLog, wType, wCategory, dwEventID, lpUserSid, wNumStrings, dwDataSize, lpStrings, lpRawData);}

//-------------------------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        constexpr const DWORD dwPatchSize = 1+sizeof(size_t);
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
            {
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
                            {
                                *pDelim = L'\0';
                                return TRUE;
                            }
                        }
                    }
                }
            }
        }
    }
    //restore kernel32!GetSystemWow64DirectoryW supposedly don't needed
    return FALSE;
}
#else
//-------------------------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, DWORD, LPVOID)
{
    return FALSE;
}
#endif
