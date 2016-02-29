#define _WIN32_WINNT 0x0600        /*RegDeleteTree*/
#include <windows.h>

int main()
{
    wchar_t wBuf[MAX_PATH+2+MAX_PATH-12/*Autoruns.exe*/];
    wchar_t *const pBuf = wBuf+1;
    const DWORD dwLen = GetModuleFileName(0, pBuf, MAX_PATH+1);
    if (dwLen >= 4 && dwLen < MAX_PATH)
    {
        wchar_t *pDelim = pBuf+dwLen;
        while (pDelim > pBuf)
        {
            if (*pDelim == L'\\')
            {
                if (pDelim <= pBuf + MAX_PATH-1-13/*\Autoruns.exe*/)
                    if (const HANDLE hProcess = GetCurrentProcess())
                    {
                        HANDLE hToken;
                        if (OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
                        {
                            struct TagTOKEN_PRIVILEGES2
                            {
                                DWORD PrivilegeCount;
                                LUID_AND_ATTRIBUTES Privileges[2];
                            };

                            bool bOk = false;
                            TagTOKEN_PRIVILEGES2 tokprv;
                            if (LookupPrivilegeValue(0, SE_RESTORE_NAME, &tokprv.Privileges[0].Luid) &&
                                    LookupPrivilegeValue(0, SE_BACKUP_NAME, &tokprv.Privileges[1].Luid))
                            {
                                tokprv.PrivilegeCount = 2;
                                tokprv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
                                tokprv.Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;
                                if (AdjustTokenPrivileges(hToken, FALSE, static_cast<TOKEN_PRIVILEGES*>(static_cast<void*>(&tokprv)), 0, 0, 0))
                                    bOk = true;
                            }
                            CloseHandle(hToken);

                            if (bOk)
                            {
                                ++pDelim;
                                wcscpy(pDelim, L"settings.dat");
                                HKEY hKey;
                                if (GetFileAttributes(pBuf) != INVALID_FILE_ATTRIBUTES)
                                {
                                    bOk = false;
                                    if (RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\Sysinternals\\AutoRuns", 0, 0, 0, KEY_WRITE, 0, &hKey, 0) == ERROR_SUCCESS)
                                    {
                                        if (RegRestoreKey(hKey, pBuf, REG_FORCE_RESTORE) == ERROR_SUCCESS)
                                            bOk = true;
                                        RegCloseKey(hKey);
                                    }
                                }

                                if (bOk)
                                {
                                    *wBuf = L'"';
                                    *pDelim = L'\0';
                                    wcscpy(wBuf+MAX_PATH+2, pBuf);
                                    wcscpy(pDelim, L"Autoruns.exe\"");

                                    PROCESS_INFORMATION pi;
                                    STARTUPINFO si;
                                    memset(&si, 0, sizeof(STARTUPINFO));
                                    si.cb = sizeof(STARTUPINFO);
                                    if (CreateProcess(0, wBuf, 0, 0, FALSE, CREATE_UNICODE_ENVIRONMENT, 0, wBuf+MAX_PATH+2, &si, &pi))
                                    {
                                        CloseHandle(pi.hThread);
                                        WaitForSingleObject(pi.hProcess, INFINITE);
                                        CloseHandle(pi.hProcess);

                                        //save settings to file
                                        wcscpy(pDelim, L"settings.dat");
                                        if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Sysinternals\\AutoRuns", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
                                        {
                                            RegSaveKey(hKey, pBuf, 0);
                                            RegCloseKey(hKey);
                                        }
                                    }

                                    //remove settings from registry
                                    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Sysinternals", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
                                    {
                                        RegDeleteTree(hKey, L"AutoRuns");
                                        RegCloseKey(hKey);
                                        if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Sysinternals", 0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey) == ERROR_SUCCESS)
                                        {
                                            DWORD dwNameLen = 0;
                                            if (RegEnumKeyEx(hKey, 0, 0, &dwNameLen, 0, 0, 0, 0) == ERROR_NO_MORE_ITEMS)
                                                RegDeleteKey(hKey, L"");
                                            RegCloseKey(hKey);
                                        }
                                    }
                                }
                            }
                        }
                    }
                break;
            }
            --pDelim;
        }
    }
    return 0;
}
