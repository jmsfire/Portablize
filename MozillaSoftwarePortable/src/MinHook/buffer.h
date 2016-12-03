#pragma once

// Size of each memory slot.
#ifdef _M_X64
    #define MEMORY_SLOT_SIZE 64
#else
    #define MEMORY_SLOT_SIZE 32
#endif

VOID   InitializeBuffer(VOID);
VOID   UninitializeBuffer(VOID);
LPVOID AllocateBuffer(LPVOID pOrigin, const LPVOID pMinimumApplicationAddress, const LPVOID pMaximumApplicationAddress, const DWORD dwAllocationGranularity);
VOID   FreeBuffer(LPVOID pBuffer);
BOOL   IsExecutableAddress(LPVOID pAddress);
