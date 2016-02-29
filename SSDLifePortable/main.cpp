#include <windows.h>

#define EXPORT //__declspec(dllexport)

enum
{
    eSize = 1+sizeof(size_t)
};

//-------------------------------------------------------------------------------------------------
LONG WINAPI RegCreateKeyExWStub(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD)
{
    return ERROR_ACCESS_DENIED;
}

//-------------------------------------------------------------------------------------------------
LONG WINAPI RegCreateKeyWStub(HKEY, LPCWSTR, PHKEY)
{
    return ERROR_ACCESS_DENIED;
}

//-------------------------------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        BYTE *pAddress = reinterpret_cast<BYTE*>(RegCreateKeyExW);
        DWORD dwOldProtect;
        if (VirtualProtect(pAddress, eSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
        {
            *pAddress = 0xE9/*jump near*/;
            size_t szOffset = reinterpret_cast<size_t>(RegCreateKeyExWStub) - (reinterpret_cast<size_t>(pAddress) + eSize);
            memcpy(pAddress+1, &szOffset, sizeof(size_t));
            DWORD dwTemp;
            if (VirtualProtect(pAddress, eSize, dwOldProtect, &dwTemp))
            {
                pAddress = reinterpret_cast<BYTE*>(RegCreateKeyW);
                if (VirtualProtect(pAddress, eSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                {
                    *pAddress = 0xE9/*jump near*/;
                    szOffset = reinterpret_cast<size_t>(RegCreateKeyWStub) - (reinterpret_cast<size_t>(pAddress) + eSize);
                    memcpy(pAddress+1, &szOffset, sizeof(size_t));
                    if (VirtualProtect(pAddress, eSize, dwOldProtect, &dwTemp))
                        return TRUE;
                }
            }
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
EXPORT WINBOOL WINAPI PlaySoundWStub(LPCWSTR, HMODULE, DWORD) {return FALSE;}
