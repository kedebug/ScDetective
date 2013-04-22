
#ifndef _PROCESS_H__
#define _PROCESS_H__

VOID 
Convert2DosDeviceName(
    PTSTR   pszSource, 
    PTSTR   pszDest, 
    int     cbszDest
    );
VOID 
GetProcessImagePath(
    HANDLE hProcess, 
    LPTSTR szFullImagePath
    );
BOOL 
AquireUserAccess(
    ULONG   ProcessId, 
    PHANDLE Handle
    );
VOID 
CloseMyHandles(
    ULONG ProcessId
    );

#endif