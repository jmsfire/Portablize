#define _WIN32_WINNT 0x0600        /*SHGetKnownFolderPath*/
#include <windows.h>
#include <shlobj.h>

#define EXPORT //__declspec(dllexport)

enum
{
    eSize = 1+sizeof(size_t),
    eLen = 48        //[48 = "\FileZilla Server\FileZilla Server Interface.xml"]
};

static wchar_t g_wBuf[MAX_PATH+1];
static size_t szPathLen;

//-------------------------------------------------------------------------------------------------
HRESULT WINAPI SHGetKnownFolderPathStub(REFKNOWNFOLDERID, DWORD, HANDLE, PWSTR *ppszPath)
{
    if (wchar_t *const pMemory = static_cast<wchar_t*>(CoTaskMemAlloc(szPathLen)))
    {
        wcscpy(pMemory, g_wBuf);
        *ppszPath = pMemory;
        return S_OK;
    }
    return S_FALSE;
}

//-------------------------------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DWORD dwTemp = GetModuleFileName(0, g_wBuf, MAX_PATH+1);
        if (dwTemp >= 33/*C:\A\FileZillaServerInterface.exe*/ && dwTemp < MAX_PATH)
            if (wchar_t *const pDelim = wcsrchr(g_wBuf, L'\\'))
                if (pDelim >= g_wBuf+4 && pDelim <= g_wBuf + MAX_PATH-1-eLen)
                {
                    BYTE *const pAddress = reinterpret_cast<BYTE*>(SHGetKnownFolderPath);
                    DWORD dwOldProtect;
                    if (VirtualProtect(pAddress, eSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
                    {
                        *pAddress = 0xE9/*jump near*/;
                        const size_t szOffset = reinterpret_cast<size_t>(SHGetKnownFolderPathStub) - (reinterpret_cast<size_t>(pAddress) + eSize);
                        memcpy(pAddress+1, &szOffset, sizeof(size_t));
                        if (VirtualProtect(pAddress, eSize, dwOldProtect, &dwTemp))
                        {
                            *pDelim = L'\0';
                            szPathLen = (pDelim-g_wBuf+1)*sizeof(wchar_t);
                            return TRUE;
                        }
                    }
                }
    }
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
EXPORT WINBOOL WINAPI PlaySoundWStub(LPCWSTR, HMODULE, DWORD) {return FALSE;}
