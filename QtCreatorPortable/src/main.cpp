//QtCreatorPortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#include <shlobj.h>

#define EXPORT //__declspec(dllexport)

static constexpr const DWORD g_dwPathMargin = 66;        //"\QtProject\qtcreator\generic-highlighter\valgrind-suppression.xml`"
static wchar_t g_wBuf[MAX_PATH+1];

//-------------------------------------------------------------------------------------------------
EXPORT LPWSTR * WINAPI CommandLineToArgvWStub(LPCWSTR lpCmdLine, int *pNumArgs)
{
    return CommandLineToArgvW(lpCmdLine, pNumArgs);
}

EXPORT WINBOOL WINAPI SHGetSpecialFolderPathWStub(HWND, LPWSTR pszPath, int, WINBOOL)
{
    const wchar_t *pSrc = g_wBuf;
    while ((*pszPath++ = *pSrc++));
    return TRUE;
}

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
            if (pDelim >= g_wBuf+4 && pDelim <= g_wBuf+MAX_PATH-g_dwPathMargin && DisableThreadLibraryCalls(hInstDll))
            {
                *pDelim = L'\0';
                return TRUE;
            }
        }
    }
    return FALSE;
}
