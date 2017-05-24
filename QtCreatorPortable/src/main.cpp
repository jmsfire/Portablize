//QtCreatorPortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#include <shlobj.h>
#ifdef _WIN64
#include "MinHook/MinHook.h"
#endif

#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 66;        //"\QtProject\qtcreator\generic-highlighter\valgrind-suppression.xml`"
static wchar_t g_wBuf[MAX_PATH+1];
static SIZE_T g_szPathBytes;

//-------------------------------------------------------------------------------------------------
EXPORT LPWSTR * WINAPI CommandLineToArgvWStub(LPCWSTR lpCmdLine, int *pNumArgs)
{
    return CommandLineToArgvW(lpCmdLine, pNumArgs);
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

//-------------------------------------------------------------------------------------------------
static LONG WINAPI RegCreateKeyExWStub(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD)
{
    return ERROR_ACCESS_DENIED;
}

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
#ifdef _WIN64
                if (MH_Initialize() == MH_OK &&
                        FCreateHook(RegCreateKeyExW, RegCreateKeyExWStub) &&
                        MH_EnableHook(reinterpret_cast<LPVOID>(RegCreateKeyExW)) == MH_OK &&
                        DisableThreadLibraryCalls(hInstDll))
                {
                    *pDelim = L'\0';
                    g_szPathBytes = (pDelim-g_wBuf+1)*sizeof(wchar_t);
                    return TRUE;
                }
#else
                constexpr const DWORD dwPatchSize = 1+sizeof(size_t);
                BYTE *const pAddress = reinterpret_cast<BYTE*>(RegCreateKeyExW);
                DWORD dwOldProtect;
                if (VirtualProtect(pAddress, dwPatchSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                {
                    const size_t szOffset = reinterpret_cast<size_t>(RegCreateKeyExWStub) - (reinterpret_cast<size_t>(pAddress) + dwPatchSize);
                    const BYTE *const pByte = static_cast<const BYTE*>(static_cast<const void*>(&szOffset));
                    pAddress[0] = 0xE9;        //jump near
                    pAddress[1] = pByte[0];
                    pAddress[2] = pByte[1];
                    pAddress[3] = pByte[2];
                    pAddress[4] = pByte[3];
                    if (VirtualProtect(pAddress, dwPatchSize, dwOldProtect, &dwTemp) && DisableThreadLibraryCalls(hInstDll))
                    {
                        *pDelim = L'\0';
                        g_szPathBytes = (pDelim-g_wBuf+1)*sizeof(wchar_t);
                        return TRUE;
                    }
                }
#endif
            }
        }
    }
#ifdef _WIN64
    else if (fdwReason == DLL_PROCESS_DETACH)
        MH_Uninitialize();
#endif
    return FALSE;
}
