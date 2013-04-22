
#ifndef _MODULE_H_
#define _MODULE_H_

#include <TlHelp32.h>
#include <NtApi.h>
#pragma comment(lib, "ntdll.lib")

BOOL
GetKernelModuleNameByAddress(
    PSTR    pszName, 
    ULONG   cbName, 
    DWORD   dwFunAddress, 
    BOOL    bFlag
    );

BOOL
GetKernelModuleBaseByName(
    PSTR    pszName,
    PULONG  SysBase
    );

BOOL
GetKernelInformation(
    PSTR    pszKernelName,
    ULONG   cbName,
    PULONG  pKernelBase
    );
BOOL
GetPsModuleNameByAddress(
    ULONG   ProcessId,
    ULONG   pfnAddress,
    LPTSTR  pszModuleName,
    ULONG   cbszModuleName
    );

#endif