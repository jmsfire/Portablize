//SpeedyFoxPortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#define WIN32_LEAN_AND_MEAN
#include <shlobj.h>
#include <wininet.h>
#include <psapi.h>

#ifndef _WIN64
#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 48;        //"\CrystalIdea Software\SpeedyFox\preferences.xml`"
static wchar_t g_wBuf[MAX_PATH+1];

//-------------------------------------------------------------------------------------------------
EXPORT DWORD WINAPI GetModuleFileNameExWStub(HANDLE hProcess, HMODULE hModule, LPWSTR lpFilename, DWORD nSize)
{
    return GetModuleFileNameExW(hProcess, hModule, lpFilename, nSize);
}

static WINBOOL WINAPI SHGetSpecialFolderPathWStub(HWND, LPWSTR pszPath, int, WINBOOL)
{
    const wchar_t *pSrc = g_wBuf;
    while ((*pszPath++ = *pSrc++));
    return TRUE;
}

static HINTERNET WINAPI InternetOpenWStub(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD)
{
    return nullptr;
}

static LONG WINAPI RegOpenKeyExWStub(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY)
{
    return ERROR_ACCESS_DENIED;
}

static LONG WINAPI RegCreateKeyExWStub(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD)
{
    return ERROR_ACCESS_DENIED;
}

//-------------------------------------------------------------------------------------------------
static bool FPatch(BYTE *const pAddress, size_t szOffset)
{
    constexpr const DWORD dwPatchSize = 1+sizeof(size_t);
    DWORD dwOldProtect;
    if (VirtualProtect(pAddress, dwPatchSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
    {
        szOffset -= reinterpret_cast<size_t>(pAddress) + dwPatchSize;
        const BYTE *pByte = static_cast<const BYTE*>(static_cast<const void*>(&szOffset));
        pAddress[0] = 0xE9;        //jump near
        pAddress[1] = pByte[0];
        pAddress[2] = pByte[1];
        pAddress[3] = pByte[2];
        pAddress[4] = pByte[3];
        DWORD dwTemp;
        if (VirtualProtect(pAddress, dwPatchSize, dwOldProtect, &dwTemp))
            return true;
    }
    return false;
}

template <typename T>
static inline bool FCreateHook(T *const pTarget, T *const pDetour)
{return FPatch(reinterpret_cast<BYTE*>(pTarget), reinterpret_cast<size_t>(pDetour));}

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
            if (pDelim >= g_wBuf+4 && pDelim <= g_wBuf+MAX_PATH-g_dwPathMargin &&
                    FCreateHook(SHGetSpecialFolderPathW, SHGetSpecialFolderPathWStub) &&
                    FCreateHook(InternetOpenW, InternetOpenWStub) &&
                    FCreateHook(RegOpenKeyExW, RegOpenKeyExWStub) &&
                    FCreateHook(RegCreateKeyExW, RegCreateKeyExWStub) &&
                    DisableThreadLibraryCalls(hInstDll))
            {
                *pDelim = L'\0';
                return TRUE;
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
