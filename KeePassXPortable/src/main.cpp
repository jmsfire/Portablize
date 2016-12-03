//KeePassXPortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#define WIN32_LEAN_AND_MEAN
#include <shlobj.h>

#ifndef _WIN64
#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 24;        //"\keepassx\keepassx?.ini`"
static wchar_t g_wBuf[MAX_PATH+1];

//-------------------------------------------------------------------------------------------------
static WINBOOL WINAPI SHGetSpecialFolderPathWStub(HWND, LPWSTR pszPath, int, WINBOOL)
{
    const wchar_t *pSrc = g_wBuf;
    while ((*pszPath++ = *pSrc++));
    return TRUE;
}

//-------------------------------------------------------------------------------------------------
EXPORT WINBOOL WINAPI CopySidStub(DWORD nDestinationSidLength, PSID pDestinationSid, PSID pSourceSid)
{return CopySid(nDestinationSidLength, pDestinationSid, pSourceSid);}
EXPORT PVOID WINAPI FreeSidStub(PSID pSid)
{return FreeSid(pSid);}
EXPORT DWORD WINAPI GetLengthSidStub(PSID pSid)
{return GetLengthSid(pSid);}
EXPORT WINBOOL WINAPI GetTokenInformationStub(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, LPVOID TokenInformation, DWORD TokenInformationLength, PDWORD ReturnLength)
{return GetTokenInformation(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength, ReturnLength);}
EXPORT WINBOOL WINAPI OpenProcessTokenStub(HANDLE ProcessHandle, DWORD DesiredAccess, PHANDLE TokenHandle)
{return OpenProcessToken(ProcessHandle, DesiredAccess, TokenHandle);}
EXPORT LONG WINAPI RegCloseKeyStub(HKEY hKey)
{return RegCloseKey(hKey);}
EXPORT LONG WINAPI RegCreateKeyExWStub(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD)
{return ERROR_ACCESS_DENIED;}
EXPORT LONG WINAPI Reg_DeleteKeyW_DeleteValueW_Stub(HKEY, LPCWSTR)
{return ERROR_ACCESS_DENIED;}
EXPORT LONG WINAPI Reg_EnumKeyExW_EnumValueW_Stub(HKEY, DWORD, LPWSTR, LPDWORD, LPDWORD, LPVOID, LPVOID, LPVOID)
{return ERROR_ACCESS_DENIED;}
EXPORT LONG WINAPI RegFlushKeyStub(HKEY)
{return ERROR_ACCESS_DENIED;}
EXPORT LONG WINAPI RegOpenKeyExWStub(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY)
{return ERROR_ACCESS_DENIED;}
EXPORT LONG WINAPI RegQueryInfoKeyWStub(HKEY, LPWSTR, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, PFILETIME)
{return ERROR_ACCESS_DENIED;}
EXPORT LONG WINAPI RegQueryValueExWStub(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD)
{return ERROR_ACCESS_DENIED;}
EXPORT LONG WINAPI RegSetValueExWStub(HKEY, LPCWSTR, DWORD, DWORD, CONST BYTE *, DWORD)
{return ERROR_ACCESS_DENIED;}

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
                BYTE *const pAddress = reinterpret_cast<BYTE*>(SHGetSpecialFolderPathW);
                DWORD dwOldProtect;
                if (VirtualProtect(pAddress, dwPatchSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                {
                    const size_t szOffset = reinterpret_cast<size_t>(SHGetSpecialFolderPathWStub) - (reinterpret_cast<size_t>(pAddress) + dwPatchSize);
                    const BYTE *const pByte = static_cast<const BYTE*>(static_cast<const void*>(&szOffset));
                    pAddress[0] = 0xE9;        //jump near
                    pAddress[1] = pByte[0];
                    pAddress[2] = pByte[1];
                    pAddress[3] = pByte[2];
                    pAddress[4] = pByte[3];
                    if (VirtualProtect(pAddress, dwPatchSize, dwOldProtect, &dwTemp) && DisableThreadLibraryCalls(hInstDll))
                    {
                        *pDelim = L'\0';
                        return TRUE;
                    }
                }
            }
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
