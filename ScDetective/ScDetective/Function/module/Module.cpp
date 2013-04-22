
//#include "../../stdafx.h"
// 使用预编译头
#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////////
//
//	功能实现： 获取函数所在的内核模块的路径
//	输入参数： dwFunAddress为要查找所在模块的函数地址；
//			  cbName为pszName的总长度；
//			  bFlag为TRUE,则是返回模块全路径，为FALSE则返回模块名字
//	输出参数： 是否调用成功
//
///////////////////////////////////////////////////////////////////////////////////
BOOL 
GetKernelModuleNameByAddress(PSTR pszName, ULONG cbName, DWORD dwFunAddress, BOOL bFlag)
{
    NTSTATUS Status;
    PSYSTEM_MODULE_INFORMATION  Modules;
    PSYSTEM_MODULE_INFORMATION_ENTRY ModuleInfo;
    PVOID Buffer;
    ULONG BufferSize = 4096;
    ULONG ReturnLength;
    ULONG NameLen;

    if (dwFunAddress < 0x80000000)  return FALSE;

retry:
    Buffer = GlobalAlloc(GPTR, BufferSize);

    if (!Buffer)  return FALSE;

    Status = ZwQuerySystemInformation(SystemModuleInformation, Buffer,
                                      BufferSize, &ReturnLength);
    if (Status == STATUS_INFO_LENGTH_MISMATCH)
    {
        GlobalFree(Buffer);
        BufferSize = ReturnLength;
        goto retry;
    }

    if (NT_SUCCESS(Status))
    {
        Modules = (PSYSTEM_MODULE_INFORMATION)Buffer;
        ModuleInfo = &(Modules->Module[0]);

        for (ULONG i = 0; i < Modules->NumberOfModules; i ++, ModuleInfo ++)
        {
            if (dwFunAddress > (DWORD)(ModuleInfo->Base) &&
                dwFunAddress < (DWORD)(ModuleInfo->Base) + ModuleInfo->Size)
            {
                if (bFlag) {
                    NameLen = lstrlenA(ModuleInfo->FullPathName);
                    if (cbName < NameLen)  goto theEnd;
                    strncpy(pszName, ModuleInfo->FullPathName, NameLen);
                } else {
                    NameLen = lstrlenA(ModuleInfo->OffsetToFileName + ModuleInfo->FullPathName);
                    if (cbName < NameLen)  goto theEnd;
                    strncpy(pszName, ModuleInfo->OffsetToFileName + ModuleInfo->FullPathName, NameLen);
                }
                GlobalFree(Buffer);
                return TRUE;
            }
        }
    }
theEnd:
    GlobalFree(Buffer);
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////////
//
//	功能实现： 根据内核模块名称获取内核模块的加载地址
//	输入参数： pszName 为输入内核模块名称
//            pBase 为返回内核模块加载地址
//	输出参数： 是否调用成功
//
///////////////////////////////////////////////////////////////////////////////////
BOOL 
GetKernelModuleBaseByName(PSTR pszName, PULONG SysBase)
{
    NTSTATUS Status;
    PSYSTEM_MODULE_INFORMATION  Modules;
    PSYSTEM_MODULE_INFORMATION_ENTRY ModuleInfo;
    PVOID Buffer;
    ULONG BufferSize = 4096;
    ULONG ReturnLength;

    if (SysBase == NULL)   return FALSE;

retry:
    Buffer = GlobalAlloc(GPTR, BufferSize);

    if (!Buffer)  return FALSE;

    Status = ZwQuerySystemInformation(SystemModuleInformation,
                                      Buffer,
                                      BufferSize,
                                      &ReturnLength
                                      );
    if (Status == STATUS_INFO_LENGTH_MISMATCH)
    {
        GlobalFree(Buffer);
        BufferSize = ReturnLength;
        goto retry;
    }

    if (NT_SUCCESS(Status))
    {
        Modules = (PSYSTEM_MODULE_INFORMATION)Buffer;
        ModuleInfo = &(Modules->Module[0]);

        for (ULONG i = 0; i < Modules->NumberOfModules; i ++, ModuleInfo ++)
        {
            if (_stricmp(ModuleInfo->OffsetToFileName + ModuleInfo->FullPathName, pszName) == 0)
            {
                *SysBase = (ULONG)(ModuleInfo->Base);
                GlobalFree(Buffer);
                return TRUE;
            }
        }
    }

    GlobalFree(Buffer);
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////
//
//	功能实现： 获取函数所在的内核模块的路径
//	输入参数： pszKernelName 为返回内核名称
//            cbName为pszName的总长度；
//	输出参数： 是否调用成功
//
///////////////////////////////////////////////////////////////////////////////////
BOOL 
GetKernelInformation(PSTR pszKernelName, ULONG cbName, PULONG KernelBase)
{
    NTSTATUS Status;
    PSYSTEM_MODULE_INFORMATION  Modules;
    PSYSTEM_MODULE_INFORMATION_ENTRY ModuleInfo;
    PVOID Buffer;
    ULONG BufferSize = 4096;
    ULONG ReturnLength;
    ULONG NameLen;

retry:
    Buffer = GlobalAlloc(GPTR, BufferSize);

    if (!Buffer)  return FALSE;

    Status = ZwQuerySystemInformation(SystemModuleInformation, Buffer, 
                                      BufferSize, &ReturnLength);
    if (Status == STATUS_INFO_LENGTH_MISMATCH)
    {
        GlobalFree(Buffer);
        BufferSize = ReturnLength;
        goto retry;
    }

    if (NT_SUCCESS(Status))
    {
        Modules = (PSYSTEM_MODULE_INFORMATION)Buffer;
        ModuleInfo = &(Modules->Module[0]);
        if (KernelBase != NULL)  KernelBase[0] = (DWORD)ModuleInfo->Base;
        NameLen = lstrlenA(ModuleInfo->OffsetToFileName + ModuleInfo->FullPathName);
        if (cbName < NameLen)  goto theEnd;
        strncpy(pszKernelName, ModuleInfo->OffsetToFileName + ModuleInfo->FullPathName, NameLen);
        GlobalFree(Buffer);
        return TRUE;
    }

theEnd:
    GlobalFree(Buffer);
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////
BOOL 
GetPsModuleNameByAddress(
            ULONG  ProcessId, 
            ULONG pfnAddress, 
            LPTSTR pszModuleName, 
            ULONG cbszModuleName
            )
{
    MODULEENTRY32 ModuleEntry;
    HANDLE hSnapShot;
    BOOL bFlag = FALSE;

    hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessId);
    ModuleEntry.dwSize = sizeof(MODULEENTRY32);
    bFlag = Module32First(hSnapShot, &ModuleEntry);
    while (bFlag) 
    {
        if ((pfnAddress >= (ULONG)ModuleEntry.modBaseAddr) &&
            (pfnAddress <= (ULONG)ModuleEntry.modBaseAddr + ModuleEntry.modBaseSize)) 
        {
            wcscpy_s(pszModuleName, cbszModuleName, ModuleEntry.szModule);
            CloseHandle(hSnapShot);
            return TRUE;
        } 
        bFlag = Module32Next(hSnapShot, &ModuleEntry);
    }
    CloseHandle(hSnapShot);
    return FALSE;
}