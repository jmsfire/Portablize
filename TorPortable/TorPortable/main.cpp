#include <windows.h>

int main()
{
    wchar_t wBuf[MAX_PATH+2+MAX_PATH-7/*tor.exe*/];
    wchar_t *const pBuf = wBuf+1;
    const DWORD dwLen = GetModuleFileName(0, pBuf, MAX_PATH+1);
    if (dwLen >= 4 && dwLen < MAX_PATH)
    {
        wchar_t *pDelim = pBuf+dwLen;
        while (pDelim > pBuf)
        {
            if (*pDelim == L'\\')
            {
                if (pDelim <= pBuf + MAX_PATH-1-8/*\tor.exe*/)
                    if (const HMODULE hMod = GetModuleHandle(L"kernel32.dll"))
                        if (const FARPROC fLoadLibraryPointer = GetProcAddress(hMod, "LoadLibraryW"))
                        {
                            *wBuf = L'"';
                            *++pDelim = L'\0';
                            wcscpy(wBuf+MAX_PATH+2, pBuf);
                            wcscpy(pDelim, L"tor.exe\"");

                            PROCESS_INFORMATION pi;
                            STARTUPINFO si;
                            si.cb = sizeof(STARTUPINFO);
                            GetStartupInfo(&si);
                            const WORD wShowWindow = (si.dwFlags & STARTF_USESHOWWINDOW) ? si.wShowWindow : ~0;
                            memset(&si, 0, sizeof(STARTUPINFO));
                            si.cb = sizeof(STARTUPINFO);
                            if (wShowWindow != static_cast<WORD>(~0))
                            {
                                si.dwFlags = STARTF_USESHOWWINDOW;
                                si.wShowWindow = wShowWindow;
                            }
                            if (CreateProcess(0, wBuf, 0, 0, FALSE, CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED, 0, wBuf+MAX_PATH+2, &si, &pi))
                            {
                                pDelim += 4/*tor.*/;
                                const size_t szSize = (pDelim+4/*dll`*/-pBuf)*sizeof(wchar_t);
                                bool bOk = false;
                                if (const LPVOID pMemory = VirtualAllocEx(pi.hProcess, 0, szSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))
                                {
                                    wcscpy(pDelim, L"dll");
                                    if (WriteProcessMemory(pi.hProcess, pMemory, pBuf, szSize, 0))
                                        if (const HANDLE hRemoteThread = CreateRemoteThread(pi.hProcess, 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(fLoadLibraryPointer), pMemory, 0, 0))
                                        {
                                            WaitForSingleObject(hRemoteThread, INFINITE);
                                            CloseHandle(hRemoteThread);
                                            bOk = true;
                                        }
                                    VirtualFreeEx(pi.hProcess, pMemory, 0, MEM_RELEASE);
                                }
                                CloseHandle(pi.hProcess);
                                if (bOk)
                                    ResumeThread(pi.hThread);
                                else
                                    TerminateThread(pi.hThread, 0);
                                CloseHandle(pi.hThread);
                            }
                        }
                break;
            }
            --pDelim;
        }
    }
    return 0;
}
