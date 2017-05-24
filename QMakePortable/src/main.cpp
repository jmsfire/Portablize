//QMakePortable
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define EXPORT //__declspec(dllexport)

//-------------------------------------------------------------------------------------------------
EXPORT LONG WINAPI RegCreateKeyExWStub(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD)
{
    return ERROR_ACCESS_DENIED;
}
EXPORT LONG WINAPI RegCloseKeyStub(HKEY hKey)
{return RegCloseKey(hKey);}
EXPORT LONG WINAPI RegDeleteKeyWStub(HKEY hKey, LPCWSTR lpSubKey)
{return RegDeleteKeyW(hKey, lpSubKey);}
EXPORT LONG WINAPI RegDeleteValueWStub(HKEY hKey, LPCWSTR lpValueName)
{return RegDeleteValueW(hKey, lpValueName);}
EXPORT LONG WINAPI RegEnumKeyExWStub(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcchName, LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcchClass, PFILETIME lpftLastWriteTime)
{return RegEnumKeyExW(hKey, dwIndex, lpName, lpcchName, lpReserved, lpClass, lpcchClass, lpftLastWriteTime);}
EXPORT LONG WINAPI RegEnumValueWStub(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{return RegEnumValueW(hKey, dwIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);}
EXPORT LONG WINAPI RegFlushKeyStub(HKEY hKey)
{return RegFlushKey(hKey);}
EXPORT LONG WINAPI RegOpenKeyExWStub(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{return RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);}
EXPORT LONG WINAPI RegQueryInfoKeyWStub(HKEY hKey, LPWSTR lpClass, LPDWORD lpcchClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen, LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen, LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime)
{return RegQueryInfoKeyW(hKey, lpClass, lpcchClass, lpReserved, lpcSubKeys, lpcbMaxSubKeyLen, lpcbMaxClassLen, lpcValues, lpcbMaxValueNameLen, lpcbMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime);}
EXPORT LONG WINAPI RegQueryValueExWStub(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{return RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);}
EXPORT LONG WINAPI RegSetValueExWStub(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, DWORD dwType, CONST BYTE *lpData, DWORD cbData)
{return RegSetValueExW(hKey, lpValueName, Reserved, dwType, lpData, cbData);}

//-------------------------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, DWORD, LPVOID)
{
    return TRUE;
}
