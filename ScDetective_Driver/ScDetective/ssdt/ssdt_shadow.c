
#include "ssdt_shadow.h"



///////////////////////////////////////////////////////////////////////////////////
//
//	功能实现：恢复 Shadow ssdt 表项
//	输入参数：AddressInfo表项内容；
//			 CsrssPEProcess 为 csrss.exe 的 EPROCESS 指针
//	输出参数：是否恢复成功
//
///////////////////////////////////////////////////////////////////////////////////
BOOLEAN 
UnHookShadowSsdtItem(
    __in PSSDT_ADDRESS AddressInfo, 
    __in PEPROCESS CsrssPEProcess
    )
{
    PSYSTEM_SERVICE_TABLE ServiceTable = NULL;
    PULONG  WriteMemoryTable = NULL;
    PMDL    pMdlServiceCall = NULL;
    KIRQL   OrigIrql;
    PKAPC_STATE ApcState;
    
    ServiceTable = GetKeServiceDescriptorTableShadow();
    if (ServiceTable == NULL || CsrssPEProcess == NULL)   return FALSE;

    ApcState = ExAllocatePoolWithTag(NonPagedPool, sizeof(KAPC_STATE), MEM_TAG);
    KeStackAttachProcess((PRKPROCESS)CsrssPEProcess, ApcState);
    
    pMdlServiceCall = IoAllocateMdl(ServiceTable->ServiceTableBase,
                                    ServiceTable->NumberOfService * sizeof(ULONG),
                                    FALSE, FALSE, NULL);
    MmBuildMdlForNonPagedPool(pMdlServiceCall);
    pMdlServiceCall->MdlFlags = pMdlServiceCall->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;
    WriteMemoryTable = MmMapLockedPagesSpecifyCache(pMdlServiceCall, KernelMode, MmCached,
                                                    NULL, FALSE, NormalPagePriority );

    KeRaiseIrql(DISPATCH_LEVEL, &OrigIrql);
    InterlockedExchange(&WriteMemoryTable[AddressInfo->nIndex], AddressInfo->FunAddress);
    KeLowerIrql(OrigIrql);

    MmUnmapLockedPages(WriteMemoryTable, pMdlServiceCall);
    IoFreeMdl(pMdlServiceCall);

    KeUnstackDetachProcess(ApcState);
    ExFreePool(ApcState);
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////
//
//	功能实现：枚举当前Shadow SSDT 表函数地址
//	输入参数：AddressInfo为输出缓冲区指针；
//			  Length为输出数据长度指针
//	输出参数：返回函数地址个数
//
///////////////////////////////////////////////////////////////////////////////////
ULONG 
GetShadowSsdtCurrentAddresses(
    PSSDT_ADDRESS   AddressInfo, 
    PULONG          Length
    )
{
    PSYSTEM_SERVICE_TABLE KeServiceDescriptorTableShadow = NULL;
    ULONG NumberOfService = 0;
    ULONG ServiceId = 0;
    PKAPC_STATE ApcState;
    ULONG i;
    
    if (AddressInfo == NULL || Length == NULL)
    {
        KdPrint(("[GetShadowSsdtCurrentAddresses] 输入参数无效"));
        return 0;
    }

    //KdPrint(("pid = %d", PsGetCurrentProcessId()));
    ServiceId = (ULONG)PsGetCurrentProcessId();

    if (!g_CsrssProcess) {
        PsLookupProcessByProcessId((PVOID)ScPsGetCsrssProcessId(), &g_CsrssProcess);
    }

    KeServiceDescriptorTableShadow = GetKeServiceDescriptorTableShadow();

    if (KeServiceDescriptorTableShadow == NULL)  
    {
        KdPrint(("[GetShadowSsdtCurrentAddresses] GetKeServiceDescriptorTableShadow failed"));
        return 0;
    }

    NumberOfService = KeServiceDescriptorTableShadow->NumberOfService;
    if (Length[0] < NumberOfService * sizeof(SSDT_ADDRESS))
    {
        Length[0] = NumberOfService * sizeof(SSDT_ADDRESS);
        return 0;
    }

    ApcState = ExAllocatePoolWithTag(NonPagedPool, sizeof(KAPC_STATE), MEM_TAG);
    KeStackAttachProcess((PRKPROCESS)g_CsrssProcess, ApcState);

    for (ServiceId = 0; ServiceId < NumberOfService; ServiceId ++)
    {
        AddressInfo[ServiceId].FunAddress = (ULONG)
            KeServiceDescriptorTableShadow->ServiceTableBase[ServiceId];
        AddressInfo[ServiceId].nIndex = ServiceId;
    }

    KeUnstackDetachProcess(ApcState);
    ExFreePool(ApcState);

    Length[0] = NumberOfService * sizeof(SSDT_ADDRESS);
    return NumberOfService;
}

///////////////////////////////////////////////////////////////////////////////////
//
//	功能实现：枚举当前Shadow SSDT 表函数地址
//	输入参数：void
//	输出参数：返回Shadow ssdt table
//
///////////////////////////////////////////////////////////////////////////////////
PSYSTEM_SERVICE_TABLE 
GetKeServiceDescriptorTableShadow(VOID)
{
    PSYSTEM_SERVICE_TABLE ShadowTable = NULL;
    ULONG   ServiceTableAddress = 0;
    PUCHAR  cPtr = NULL; 

    for (cPtr = (PUCHAR)KeAddSystemServiceTable;
         cPtr < (PUCHAR)KeAddSystemServiceTable + PAGE_SIZE;
         cPtr += 1 )
    {
        if (!MmIsAddressValid(cPtr))  continue;

        ServiceTableAddress = *(PULONG)cPtr;
        if (!MmIsAddressValid((PVOID)ServiceTableAddress)) continue;

        if (memcmp((PVOID)ServiceTableAddress, &KeServiceDescriptorTable, 16) == 0)
        {
            if ((PVOID)ServiceTableAddress == &KeServiceDescriptorTable) continue;
            ShadowTable = (PSYSTEM_SERVICE_TABLE)ServiceTableAddress;
            ShadowTable ++;
            return ShadowTable;
        }
    }
    return NULL;
}
//
// 方法2，通过硬编码实现，不通用
// 
PSYSTEM_SERVICE_TABLE 
GetKeServiceDescriptorTableShadow_2(VOID)
/*++
    805ba5a3 8d8840a65580    lea    ecx,nt!KeServiceDescriptorTableShadow (8055a640)[eax]
    805ba5a9 833900          cmp    dword ptr [ecx],0
--*/
{
    PSYSTEM_SERVICE_TABLE ShadowTable;
    ULONG   ServiceTableAddress;
    PUCHAR  cPtr, pOpcode;
    ULONG   Length = 0;

    for (cPtr = (PUCHAR)KeAddSystemServiceTable;
         cPtr < (PUCHAR)KeAddSystemServiceTable + PAGE_SIZE;
         cPtr += Length )
    {
        if (!MmIsAddressValid(cPtr))  return NULL;

        Length = SizeOfCode(cPtr, &pOpcode);

        if (!Length || (Length == 1 && *pOpcode == 0xC3)) return NULL;

        if (*(PUSHORT)pOpcode == 0x888D)
        {
            ServiceTableAddress = *(PULONG)(pOpcode + 2);
            ShadowTable = (PSYSTEM_SERVICE_TABLE)ServiceTableAddress;
            ShadowTable ++;
            return ShadowTable;
        }
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////////
//
//	功能实现：返回SHADOW SSDT表中的服务函数个数
//	输入参数：无
//	输出参数：返回函数个数
//
///////////////////////////////////////////////////////////////////////////////////
ULONG 
GetShadowSsdtServiceNumber(VOID)
{
    PSYSTEM_SERVICE_TABLE ServiceTable = NULL;

    ServiceTable = (PSYSTEM_SERVICE_TABLE)GetKeServiceDescriptorTableShadow();
    if (!ServiceTable)  return 0;
    return ServiceTable->NumberOfService;
}

