//APIMonitorPortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#define WIN32_LEAN_AND_MEAN
#include <oledlg.h>
#ifdef _WIN64
#include "MinHook/MinHook.h"
#endif

#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 63;        //"\apimonitor-definitions-{01234567-abcd-abcd-abcd-0123456789ab}`"
static constexpr const DWORD g_dwPatchSize = 1+sizeof(size_t);
static wchar_t g_wBuf[MAX_PATH+1];

#ifndef _WIN64
//-------------------------------------------------------------------------------------------------
static inline void FCopyMemory(BYTE *pDst, const BYTE *pSrc, DWORD dwSize)
{
    while (dwSize--)
        *pDst++ = *pSrc++;
}
#endif

//-------------------------------------------------------------------------------------------------
EXPORT UINT OleUIBusyWStub(LPOLEUIBUSYW)
{
    return OLEUI_FALSE;
}

//-------------------------------------------------------------------------------------------------
static DWORD WINAPI GetTempPathWStub(DWORD, LPWSTR lpBuffer)
{
    const wchar_t *pSrc = g_wBuf;
    while ((*lpBuffer++ = *pSrc++));
    return pSrc-g_wBuf-1;
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
            {
#ifdef _WIN64
                if (MH_Initialize() == MH_OK &&
                        FCreateHook(GetTempPathW, GetTempPathWStub) &&
                        MH_EnableHook(reinterpret_cast<LPVOID>(GetTempPathW)) == MH_OK &&
                        DisableThreadLibraryCalls(hInstDll))
                {
                    pDelim[1] = L'\0';
                    return TRUE;
                }
#else
                BYTE *const pAddress = reinterpret_cast<BYTE*>(GetTempPathW);
                DWORD dwOldProtect;
                if (VirtualProtect(pAddress, g_dwPatchSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                {
                    FCopyMemory(btGetTempPathW, reinterpret_cast<BYTE*>(GetTempPathW), g_dwPatchSize);
                    const size_t szOffset = reinterpret_cast<size_t>(GetTempPathWStub) - (reinterpret_cast<size_t>(pAddress) + g_dwPatchSize);
                    const BYTE *const pByte = static_cast<const BYTE*>(static_cast<const void*>(&szOffset));
                    pAddress[0] = 0xE9;        //jump near
                    pAddress[1] = pByte[0];
                    pAddress[2] = pByte[1];
                    pAddress[3] = pByte[2];
                    pAddress[4] = pByte[3];
                    if (VirtualProtect(pAddress, g_dwPatchSize, dwOldProtect, &dwTemp) && DisableThreadLibraryCalls(hInstDll))
                    {
                        pDelim[1] = '\0';
                        return TRUE;
                    }
                }
#endif
            }
        }
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
#ifdef _WIN64
        MH_Uninitialize();
#else
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
#endif
    }
    return FALSE;
}
