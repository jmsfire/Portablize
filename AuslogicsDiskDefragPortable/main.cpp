#define _WIN32_WINNT 0x0600        /*RegDeleteTree*/
#include <winsock2.h>
#include <shlobj.h>

#define EXPORT //__declspec(dllexport)

enum
{
    eSize = 1+sizeof(size_t),
    eLen = 47        //[47 = "\Auslogics\DiskDefrag\6.x\DD_ExclusionsList.dat"]
};

static wchar_t g_wBuf[MAX_PATH+1];
static wchar_t *g_pDelim;
static HANDLE g_hProcess;

//-------------------------------------------------------------------------------------------------
WINBOOL WINAPI SHGetSpecialFolderPathWStub(HWND hWnd, LPWSTR pszPath, int csidl, WINBOOL)
{
    if (csidl == CSIDL_COMMON_APPDATA)
    {
        wcscpy(pszPath, g_wBuf);
        return TRUE;
    }
    return SHGetFolderPathW(hWnd, csidl, 0, SHGFP_TYPE_CURRENT, pszPath) == S_OK ? TRUE : FALSE;
}

//-------------------------------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DWORD dwTemp = GetModuleFileName(0, g_wBuf, MAX_PATH+1);
        if (dwTemp >= 19/*C:\A\DiskDefrag.exe*/ && dwTemp < MAX_PATH &&
                (g_pDelim = wcsrchr(g_wBuf, L'\\')) &&
                g_pDelim >= g_wBuf+4 && g_pDelim <= g_wBuf + MAX_PATH-1-eLen)
        {
            BYTE *const pAddress = reinterpret_cast<BYTE*>(SHGetSpecialFolderPathW);
            DWORD dwOldProtect;
            if (VirtualProtect(pAddress, eSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
            {
                *pAddress = 0xE9/*jump near*/;
                const size_t szOffset = reinterpret_cast<size_t>(SHGetSpecialFolderPathWStub) - (reinterpret_cast<size_t>(pAddress) + eSize);
                memcpy(pAddress+1, &szOffset, sizeof(size_t));
                if (VirtualProtect(pAddress, eSize, dwOldProtect, &dwTemp) && (g_hProcess = GetCurrentProcess()))
                {
                    //restore settings from file
                    bool bOk = true;
                    wcscpy(g_pDelim, L"\\settings.dat");
                    if (GetFileAttributes(g_wBuf) != INVALID_FILE_ATTRIBUTES)
                    {
                        bOk = false;
                        HANDLE hToken;
                        if (OpenProcessToken(g_hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
                        {
                            TOKEN_PRIVILEGES tokprv;
                            if (LookupPrivilegeValue(0, SE_RESTORE_NAME, &tokprv.Privileges[0].Luid))
                            {
                                tokprv.PrivilegeCount = 1;
                                tokprv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
                                if (AdjustTokenPrivileges(hToken, FALSE, &tokprv, 0, 0, 0))
                                    bOk = true;
                            }
                            CloseHandle(hToken);

                            if (bOk)
                            {
                                bOk = false;
                                HKEY hKey;
                                if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Auslogics\\DiskDefrag\\6.x\\Settings", 0, 0, 0, KEY_WRITE, 0, &hKey, 0) == ERROR_SUCCESS)
                                {
                                    if (RegRestoreKey(hKey, g_wBuf, REG_FORCE_RESTORE) == ERROR_SUCCESS)
                                        bOk = true;
                                    RegCloseKey(hKey);
                                }

                                if (bOk)
                                {
                                    bOk = false;
                                    if (OpenProcessToken(g_hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
                                    {
                                        if (LookupPrivilegeValue(0, SE_RESTORE_NAME, &tokprv.Privileges[0].Luid))
                                        {
                                            tokprv.PrivilegeCount = 1;
                                            tokprv.Privileges[0].Attributes = 0;
                                            if (AdjustTokenPrivileges(hToken, FALSE, &tokprv, 0, 0, 0))
                                                bOk = true;
                                        }
                                        CloseHandle(hToken);
                                    }
                                }
                            }
                        }
                    }

                    if (bOk)
                    {
                        *g_pDelim = L'\0';
                        return TRUE;
                    }
                }
            }
        }
        g_hProcess = 0;
    }
    else if (fdwReason == DLL_PROCESS_DETACH && g_hProcess)
    {
        //save settings to file
        wcscpy(g_pDelim, L"\\settings.dat");
        HKEY hKey;
        if (GetFileAttributes(g_wBuf) == INVALID_FILE_ATTRIBUTES || DeleteFile(g_wBuf))
        {
            HANDLE hToken;
            if (OpenProcessToken(g_hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
            {
                bool bOk = false;
                TOKEN_PRIVILEGES tokprv;
                if (LookupPrivilegeValue(0, SE_BACKUP_NAME, &tokprv.Privileges[0].Luid))
                {
                    tokprv.PrivilegeCount = 1;
                    tokprv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
                    if (AdjustTokenPrivileges(hToken, FALSE, &tokprv, 0, 0, 0))
                        bOk = true;
                }
                CloseHandle(hToken);

                if (bOk && RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Auslogics\\DiskDefrag\\6.x\\Settings", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
                {
                    RegSaveKey(hKey, g_wBuf, 0);
                    RegCloseKey(hKey);
                }
            }
        }

        //remove settings from registry
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Auslogics", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
        {
            RegDeleteTree(hKey, L"DiskDefrag");
            RegDeleteTree(hKey, L"Google Analytics Package");
            RegCloseKey(hKey);
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Auslogics", 0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey) == ERROR_SUCCESS)
            {
                DWORD dwNameLen = 0;
                if (RegEnumKeyEx(hKey, 0, 0, &dwNameLen, 0, 0, 0, 0) == ERROR_NO_MORE_ITEMS)
                    RegDeleteKey(hKey, L"");
                RegCloseKey(hKey);
            }
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
EXPORT SOCKET WSAAPI acceptStub(SOCKET, struct sockaddr *, int *) {return INVALID_SOCKET;}
EXPORT int WSAAPI bindStub(SOCKET, const struct sockaddr *, int) {return SOCKET_ERROR;}
EXPORT int WSAAPI closesocketStub(SOCKET) {return SOCKET_ERROR;}
EXPORT int WSAAPI connectStub(SOCKET, const struct sockaddr *, int) {return SOCKET_ERROR;}
EXPORT struct hostent *WSAAPI gethostbyaddrStub(const char *, int, int) {return 0;}
EXPORT struct hostent *WSAAPI gethostbynameStub(const char *) {return 0;}
EXPORT int WSAAPI gethostnameStub(char *, int) {return SOCKET_ERROR;}
EXPORT int WSAAPI getpeernameStub(SOCKET, struct sockaddr *, int *) {return SOCKET_ERROR;}
EXPORT struct servent *WSAAPI getservbynameStub(const char *, const char *) {return 0;}
EXPORT int WSAAPI getsocknameStub(SOCKET, struct sockaddr *, int *) {return 0;}
EXPORT int WSAAPI getsockoptStub(SOCKET, int, int, char *, int *) {return 0;}
EXPORT u_short WSAAPI htonsStub(u_short) {return 0;}
EXPORT unsigned __LONG32 WSAAPI inet_addrStub(const char *) {return INADDR_NONE;}
EXPORT char *WSAAPI inet_ntoaStub(struct in_addr) {return 0;}
EXPORT int WSAAPI ioctlsocketStub(SOCKET, __LONG32, u_long *) {return SOCKET_ERROR;}
EXPORT int WSAAPI listenStub(SOCKET, int) {return SOCKET_ERROR;}
EXPORT u_short WSAAPI ntohsStub(u_short) {return 0;}
EXPORT int WSAAPI recvStub(SOCKET, char *, int, int) {return SOCKET_ERROR;}
EXPORT int WSAAPI recvfromStub(SOCKET, char *, int, int, struct sockaddr *, int *) {return SOCKET_ERROR;}
EXPORT int WSAAPI selectStub(int, fd_set *, fd_set *, fd_set *, const PTIMEVAL) {return SOCKET_ERROR;}
EXPORT int WSAAPI sendStub(SOCKET, const char *, int, int) {return SOCKET_ERROR;}
EXPORT int WSAAPI sendtoStub(SOCKET, const char *, int, int, const struct sockaddr *, int) {return SOCKET_ERROR;}
EXPORT int WSAAPI setsockoptStub(SOCKET, int, int, const char *, int) {return SOCKET_ERROR;}
EXPORT SOCKET WSAAPI socketStub(int, int, int) {return INVALID_SOCKET;}
EXPORT HANDLE WSAAPI WSAAsyncGetHostByNameStub(HWND, u_int, const char *, char *, int) {return 0;}
EXPORT HANDLE WSAAPI WSAAsyncGetServByNameStub(HWND, u_int, const char *, const char *, char *, int) {return 0;}
EXPORT int WSAAPI WSAAsyncSelectStub(SOCKET, HWND, u_int, __LONG32) {return SOCKET_ERROR;}
EXPORT int WSAAPI WSACancelAsyncRequestStub(HANDLE) {return SOCKET_ERROR;}
EXPORT int WSAAPI WSACleanupStub(void) {return SOCKET_ERROR;}
EXPORT int WSAAPI WSAGetLastErrorStub(void) {return WSAEACCES;}
EXPORT int WSAAPI WSAStartupStub(WORD, LPWSADATA) {return WSASYSNOTREADY;}
