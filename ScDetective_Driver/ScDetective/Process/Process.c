
#include "Process.h"

//////////////////////////////////////////////////////////////////////////

ULONG ScObGetObjectType(PVOID Object)
{
    UNICODE_STRING FuncName;
    pFnObGetObjectType pfnObGetObjectType = NULL;

    if (!MmIsAddressValid(Object))  return 0;

    if (g_WindowsVersion < WINDOWS_VERSION_7) {
        return *(PULONG)((ULONG)Object - OBJECT_HEADER_SIZE + OBJECT_TYPE_OFFSET);
    } else {
        RtlInitUnicodeString(&FuncName, L"ObGetObjectType"); 
        pfnObGetObjectType = MmGetSystemRoutineAddress(&FuncName); 
        return (ULONG)pfnObGetObjectType(Object);
    }
}

BOOLEAN JudgeAddressProcess(ULONG address)
{
    ULONG ObjectTypeProcess;
    ULONG ObjectHeaderAddress;
    ULONG ObjectType;
    
    if (ScmValidPage(address - offset_Process_Peb) != VALID_PAGE)   return FALSE;

    ObjectHeaderAddress = address - offset_Process_Peb - OBJECT_HEADER_SIZE;
    ObjectTypeProcess = ScObGetObjectType(g_SystemProcess);

    if (ScmValidPage(ObjectHeaderAddress) == VALID_PAGE) {
        ObjectType = ScObGetObjectType((PVOID)(address - offset_Process_Peb));
        if (ObjectType == ObjectTypeProcess) return TRUE;
    }
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////

ULONG ScPsGetPsActiveProcessHead()
{
    static ULONG PsActiveProcessHead = 0;
    PDBGKD_GET_VERSION64 KdVersionBlock = NULL;
    PKDDEBUGGER_DATA64 DebuggerData = NULL;
    ULONG Time = 4; 

    if (PsActiveProcessHead != 0)  return PsActiveProcessHead;

_retry:
    KeSetSystemAffinityThread(1);
    __asm {
        mov     eax, fs:[0x1c]
        mov     eax, [eax+0x34]
        mov     KdVersionBlock, eax
    }
    KeRevertToUserAffinityThread();

    if (KdVersionBlock == NULL && 
        Time != 0)  {
            Time --;  goto _retry;
    } 
    if (KdVersionBlock != NULL) {
        DebuggerData = (PKDDEBUGGER_DATA64)*((PULONG)KdVersionBlock->DebuggerDataList);
        PsActiveProcessHead = (ULONG)DebuggerData->PsActiveProcessHead;
        KdPrint(("[SpsGetPsActiveProcessHead][FS] PsActiveProcessHead : 0x%08x", PsActiveProcessHead));
        return PsActiveProcessHead;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////

ULONG ScPsGetPspCidTable()
{
    static ULONG PspCidTable = 0;
    UNICODE_STRING NameString;
    ULONG FunctionAddress = 0;
    ULONG address;
    PDBGKD_GET_VERSION64 KdVersionBlock = NULL;
    PKDDEBUGGER_DATA64 DebuggerData = NULL;
    ULONG Time = 4; 

    if (PspCidTable != 0)  return PspCidTable;

_retry:
    KeSetSystemAffinityThread(1);
    __asm {
        mov     eax, fs:[0x1c]
        mov     eax, [eax+0x34]
        mov     KdVersionBlock, eax
    }
    KeRevertToUserAffinityThread();

    if (KdVersionBlock == NULL && 
        Time != 0)  {
        Time --;  goto _retry;
    } 
    if (KdVersionBlock != NULL) {
        DebuggerData = (PKDDEBUGGER_DATA64)*((PULONG)KdVersionBlock->DebuggerDataList);
        PspCidTable = (ULONG)DebuggerData->PspCidTable;
        KdPrint(("[ScPsGetPspCidTable][FS] PspCidTable : 0x%08x", PspCidTable));
        return PspCidTable;
    }
    
    RtlInitUnicodeString(&NameString, L"PsLookupProcessByProcessId");
    FunctionAddress = (ULONG)MmGetSystemRoutineAddress(&NameString);

    if (g_WindowsVersion == WINDOWS_VERSION_7) 
    {
       for (address = FunctionAddress; 
            address < FunctionAddress + PAGE_SIZE; 
            address ++ ) 
       {
            if (((*(PUSHORT)address) == 0x3D8B) && 
                ((*(PUCHAR)(address + 6)) == 0xE8)) 
            {	
                PspCidTable = *(PULONG)(address + 2);
                KdPrint(("[GetPspCidTable] Get PspCidTable : 0x%08x", PspCidTable));
                return PspCidTable;
            }	
        }
    } else if (g_WindowsVersion == WINDOWS_VERSION_XP) {

        for (address = FunctionAddress; 
             address < FunctionAddress + PAGE_SIZE; 
             address ++ )
        {
            if (((*(PUSHORT)address) == 0x35FF) && 
                ((*(PUCHAR)(address + 6)) == 0xE8))
            {	
                PspCidTable = *(PULONG)(address + 2);
                KdPrint(("[GetPspCidTable] Get PspCidTable : 0x%08x", PspCidTable));
                return PspCidTable;
            }	
        }
    }
    KdPrint(("[GetPspCidTable] Get PspCidTable fs: 0x%08x", PspCidTable));
    return PspCidTable;
}

//////////////////////////////////////////////////////////////////////////

NTSTATUS GetProcessInformation(PEPROCESS EProcess, PPROCESS_INFO ReadyProcess)
{
    PUNICODE_STRING NameString = NULL;
    
    ASSERT(EProcess && ReadyProcess);

    if (!MmIsAddressValid(EProcess) || 
        !MmIsAddressValid(ReadyProcess)) {
        return STATUS_INVALID_ADDRESS;
    }
    
    if (EProcess != g_SystemProcess) {

        NameString = ExAllocatePoolWithTag(NonPagedPool, 0x208, MEM_TAG);
        NameString->Buffer = (PWCH)((ULONG)NameString + 8);
        NameString->Length = 0;
        NameString->MaximumLength = (USHORT)258;

        ScPsGetProcessImagePath((PEPROCESS)EProcess, NameString);
        RtlCopyMemory(ReadyProcess->ImagePath, NameString->Buffer, NameString->Length);
        ExFreePoolWithTag(NameString, MEM_TAG);

    } else {

        RtlCopyMemory(ReadyProcess->ImagePath, L"System", sizeof(L"System"));
    }

    ReadyProcess->EProcess = (ULONG)EProcess;
    ReadyProcess->InheritedProcessId = *(PULONG)((PUCHAR)EProcess + offset_Process_InheritedProcessId);
    ReadyProcess->UniqueProcessId = *(PULONG)((PUCHAR)EProcess + offset_Process_UniqueProcessId);

    return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

VOID BrowseTable_L3(ULONG TableAddress)
{
    ULONG ObjectTypeProcess;
    ULONG ObjectType;
    ULONG Object;
    ULONG HandleItem = 511;
    PPROCESS_INFO FoundProcess = NULL;

    ObjectTypeProcess = ScObGetObjectType(g_SystemProcess);

    while (HandleItem) {

        TableAddress += 8;

        Object = ((PULONG)TableAddress)[0];
        Object &= 0xFFFFFFF8;

        if (Object == 0)  continue;
        ObjectType = ScObGetObjectType((PVOID)Object);

        if (ObjectType == ObjectTypeProcess) {

            if (((((PULONG)(Object + offset_Process_CreateTime))[0] != 0 ||
                  ((PULONG)(Object + offset_Process_CreateTime))[1] != 0) &&
                  ((PULONG)(Object + offset_Process_ExitTime))[0] == 0  &&
                  ((PULONG)(Object + offset_Process_ExitTime))[1] == 0) ||
                  ((PULONG)(Object + offset_Process_InheritedProcessId))[0] == 0) {

                FoundProcess = ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESS_INFO), MEM_TAG);
                RtlZeroMemory(FoundProcess, sizeof(PROCESS_INFO));
                GetProcessInformation((PEPROCESS)Object, FoundProcess);

                InsertTailList(&g_ProcessListHead->ProcessListHead, &FoundProcess->ProcessLink);
                g_ProcessListHead->NumberOfProcesses ++;
            }
        }
        -- HandleItem;
    }
}

//////////////////////////////////////////////////////////////////////////

VOID BrowseTable_L2(ULONG TableAddress)
{
    while ((*(PULONG)TableAddress) != 0)
    {
        BrowseTable_L3(*(PULONG)TableAddress);
        TableAddress += 4;
    }
}

//////////////////////////////////////////////////////////////////////////

VOID BrowseTable_L1(ULONG TableAddress)
{
    while ((*(PULONG)TableAddress) != 0)
    {
        BrowseTable_L2(*(PULONG)TableAddress);
        TableAddress += 4;
    }
}

//////////////////////////////////////////////////////////////////////////

ULONG GetPebCommonAddress()
{
    ULONG address;
    PEPROCESS Process;

    //
    //由于system进程的peb总是零 只有到其他进程去找了
    //
    Process = (PEPROCESS)((ULONG)((PLIST_ENTRY)
        ((ULONG)g_SystemProcess + offset_Process_ActiveProcessLinks))->Flink - offset_Process_ActiveProcessLinks);
    address = *(PULONG)((ULONG)Process + offset_Process_Peb);

    return (address & 0xFFFF0000);  
}

//////////////////////////////////////////////////////////////////////////

VOID EnumerateProcessByPspCidTable()
{
    ULONG PspCidTable;
    ULONG HandleTable;
    ULONG TableCode;
    ULONG TableFlag;
    //
    // Parse PspCidTable
    //
    ASSERT (g_PspCidTable);

    PspCidTable =  g_PspCidTable;
    HandleTable = *(PULONG)PspCidTable;
    TableCode   = *(PULONG)HandleTable;
    TableFlag   =  TableCode & 3;
    TableCode   &= 0xFFFFFFFC;

    switch (TableFlag)
    {
    case 0:
        BrowseTable_L3(TableCode);
        break;
    case 1:
        BrowseTable_L2(TableCode);
        break;
    case 2:
        BrowseTable_L1(TableCode);
        break;
    }
}

//////////////////////////////////////////////////////////////////////////

VOID EnumerateProcessByMemorySearch()
{
    ULONG PebAddress = GetPebCommonAddress();
    ULONG NowAddress, address;
    ULONG ReturnValue;
    ULONG ObjectTable;
    ULONG ObjectProcess;
    ULONG EProcess;
    PPROCESS_INFO HiddenProcess;
    PPROCESS_INFO ReadyProcess;
    PLIST_ENTRY p;
    BOOL LableFind = FALSE;

    for (NowAddress = ScmNonPagedPoolStart; NowAddress < ScmNonPagedPoolEnd0G; NowAddress += 4)
    {
        __try {
            ReturnValue = ScmValidPage(NowAddress);

            if (ReturnValue == PTE_INVALID) {
                NowAddress -= 4; 
                NowAddress += PTE_SIZE;
                continue;
            } else if (ReturnValue == PDE_INVALID) {
                NowAddress -= 4; 
                NowAddress += PDE_SIZE; 
                continue;
            }

            address = *(PULONG)NowAddress;
            if ((address & 0xFFFF0000) != PebAddress)  continue;

            if (JudgeAddressProcess(NowAddress)) {

                EProcess = NowAddress - offset_Process_Peb;

                for (p = g_ProcessListHead->ProcessListHead.Flink;
                     p != &g_ProcessListHead->ProcessListHead;
                     p = p->Flink) {
                    ReadyProcess = CONTAINING_RECORD(p, PROCESS_INFO, ProcessLink);
                    if (ReadyProcess->EProcess == EProcess) {
                        LableFind = TRUE;  break;
                    }
                }

                if (LableFind == TRUE) {
                    LableFind = FALSE;
                } else if ((((PULONG)(EProcess + offset_Process_CreateTime))[0] != 0 ||
                            ((PULONG)(EProcess + offset_Process_CreateTime))[1] != 0) &&
                            ((PULONG)(EProcess + offset_Process_ExitTime))[0] == 0 &&
                            ((PULONG)(EProcess + offset_Process_ExitTime))[1] == 0 &&
                            EProcess != (ULONG)g_IdleProcess) {

                    ObjectTable = ((PULONG)((ULONG)EProcess + offset_Process_ObjectTable))[0];
                    if (!MmIsAddressValid((PVOID)(ObjectTable + offset_HandleTable_QuotaProcess))) goto _end;
                    ObjectProcess = ((PULONG)(ObjectTable + offset_HandleTable_QuotaProcess))[0];
                    if (EProcess != ObjectProcess)  goto _end;

                    // 发现抹 PspCidTable 的进程
                    HiddenProcess = ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESS_INFO), MEM_TAG);
                    RtlZeroMemory(HiddenProcess, sizeof(PROCESS_INFO));
                    GetProcessInformation((PEPROCESS)EProcess, HiddenProcess);
                    HiddenProcess->bHidden = TRUE;

                    g_ProcessListHead->NumberOfProcesses ++;
                    InsertTailList(&g_ProcessListHead->ProcessListHead, &HiddenProcess->ProcessLink);
                }
_end:           NowAddress += EPROCESS_SIZE;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) { continue; }
    }
}

//////////////////////////////////////////////////////////////////////////

PPROCESS_LIST_HEAD ScPsQuerySystemProcessList()
{
    PPROCESS_INFO IdleProcess = NULL;

    if (g_ProcessListHead)  return g_ProcessListHead;

    g_ProcessListHead = ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESS_LIST_HEAD), MEM_TAG);

    InitializeListHead(&g_ProcessListHead->ProcessListHead);
    g_ProcessListHead->NumberOfProcesses = 0;

    //
    // Insert system idle process
    //
    if (g_IdleProcess == NULL)  goto _Ignore;

    IdleProcess = ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESS_INFO), MEM_TAG);
    RtlZeroMemory(IdleProcess, sizeof(PROCESS_INFO));
    IdleProcess->EProcess = (ULONG)g_IdleProcess;
    RtlCopyMemory(IdleProcess->ImagePath, L"Idle", sizeof(L"Idle"));
    IdleProcess->InheritedProcessId = *(PULONG)((PUCHAR)g_IdleProcess + offset_Process_InheritedProcessId);
    IdleProcess->UniqueProcessId = *(PULONG)((PUCHAR)g_IdleProcess + offset_Process_UniqueProcessId);
    InsertTailList(&g_ProcessListHead->ProcessListHead, &IdleProcess->ProcessLink);

    g_ProcessListHead->NumberOfProcesses ++;

_Ignore:
    EnumerateProcessByPspCidTable();
    EnumerateProcessByMemorySearch();

    return g_ProcessListHead;
}

//////////////////////////////////////////////////////////////////////////

ULONG ExCopyProcessList2Buffer(PPROCESS_INFO ProcessInfo)
{
    PPROCESS_INFO tempProcess;
    ULONG ReturnLength = 0;

    if (g_ProcessListHead == NULL)  return 0;

    while (!IsListEmpty(&g_ProcessListHead->ProcessListHead))
    {
        tempProcess = (PPROCESS_INFO)RemoveHeadList(&g_ProcessListHead->ProcessListHead);
        RtlCopyMemory(ProcessInfo, tempProcess, sizeof(PROCESS_INFO));
        ExFreePoolWithTag(tempProcess, MEM_TAG);
        ProcessInfo ++;
        ReturnLength ++;
    }

    ExFreePoolWithTag(g_ProcessListHead, MEM_TAG);
    g_ProcessListHead = NULL;
    return ReturnLength * sizeof(PROCESS_INFO);
}

//////////////////////////////////////////////////////////////////////////
NTSTATUS 
ScPsGetProcessImagePath(PEPROCESS Process, PUNICODE_STRING NameString)
{
    NTSTATUS ntStatus;
    HANDLE HandleProcess;
    ULONG ReturnLength;
    PVOID SectionObject;
    PFILE_OBJECT FilePointer;

    ASSERT(Process != NULL && NameString != NULL);

    if (!MmIsAddressValid(NameString) || 
        !MmIsAddressValid(Process)) {
        return STATUS_ACCESS_DENIED; 
    }
    // 
    // Get FilePointer from SectionObject
    // The struct is VALID_PAGE on both xp and windows 7
    //
    SectionObject = *(PVOID*)((ULONG)Process + offset_Process_SectionObject);
    if (!MmIsAddressValid(SectionObject))   return STATUS_ACCESS_DENIED;

    FilePointer = (PFILE_OBJECT)(*((PULONG)SectionObject + 5));
    if (!MmIsAddressValid(FilePointer))      return STATUS_ACCESS_DENIED;

    FilePointer = *(PFILE_OBJECT *)FilePointer;
    if (!MmIsAddressValid(FilePointer))      return STATUS_ACCESS_DENIED;

    FilePointer = *(PFILE_OBJECT *)((ULONG)FilePointer + 0x24);
    if (!MmIsAddressValid(FilePointer))      return STATUS_ACCESS_DENIED;

    FilePointer = (PFILE_OBJECT)(((ULONG)FilePointer) & 0xFFFFFFF8);

    ntStatus = ObReferenceObject(FilePointer);
    if (!NT_SUCCESS(ntStatus))  return STATUS_ACCESS_DENIED;
    
    ntStatus = ScfsQueryFileNameString(FilePointer, NameString);
    ObDereferenceObject(FilePointer);

    if (NT_SUCCESS(ntStatus))  return ntStatus;

    // 
    // If not succeed, use ZwQueryInformationProcess
    //
    __try {
        ntStatus = ObOpenObjectByPointer(Process, OBJ_INHERIT, NULL, 
                                         0, *PsProcessType, 
                                         ExGetPreviousMode(), &HandleProcess);

        if (!NT_SUCCESS(ntStatus))  return ntStatus;

        ntStatus = ZwQueryInformationProcess(HandleProcess, 
                                         ProcessImageFileName, 
                                         NameString,
                                         NameString->MaximumLength + \
                                            sizeof(UNICODE_STRING),
                                         &ReturnLength);

        ObCloseHandle(HandleProcess, ExGetPreviousMode());
    } 
    __except (EXCEPTION_EXECUTE_HANDLER) { ntStatus = STATUS_UNSUCCESSFUL; }
    
    return ntStatus;
}

//////////////////////////////////////////////////////////////////////////

PTHREAD_LIST_HEAD 
ScPsQueryProcessThreadList(PEPROCESS EProcess)
{
    PLIST_ENTRY ListBegin = NULL;
    PLIST_ENTRY ListEntry = NULL;
    PTHREAD_INFO FoundThread = NULL;
    ULONG EThread;
    PCLIENT_ID ClientId = NULL;
    KTHREAD_STATE State;

    if (g_ThreadListHead)   return g_ThreadListHead;

    __try {
        g_ThreadListHead = ExAllocatePoolWithTag(NonPagedPool, sizeof(THREAD_LIST_HEAD), MEM_TAG);

        InitializeListHead(&g_ThreadListHead->ThreadListHead);
        g_ThreadListHead->NumberOfThread = 0;

        ListBegin = (PLIST_ENTRY)((ULONG)EProcess + offset_Process_ThreadListHead);

        for (ListEntry = ListBegin->Flink; ListEntry != ListBegin; ListEntry = ListEntry->Flink)
        {
            FoundThread = ExAllocatePoolWithTag(NonPagedPool, sizeof(THREAD_INFO), MEM_TAG);
            EThread = (ULONG)ListEntry - offset_Thread_ThreadListEntry;

            RtlZeroMemory(FoundThread, sizeof(THREAD_INFO));

            FoundThread->EThread = EThread;
            FoundThread->Priority = *(PSCHAR)(EThread + offset_Thread_Priority);
            FoundThread->Win32StartAddress = *(PULONG)(EThread + offset_Thread_StartAddress);
            FoundThread->Teb = *(PULONG)(EThread + offset_Thread_Teb);
            FoundThread->ContextSwitches = *(PULONG)(EThread + offset_Thread_ContextSwitches);

            State = *(PUCHAR)(EThread + offset_Thread_State);

            if (State <= 8 && State >= 0)
                RtlCopyMemory(FoundThread->State, ThreadState[State], 3 * 2);
            else
                RtlCopyMemory(FoundThread->State, L"-", sizeof(L"-"));

            ClientId = (PCLIENT_ID)(EThread + offset_Thread_Cid);
            FoundThread->ThreadId = (ULONG)ClientId->UniqueThread;

            g_ThreadListHead->NumberOfThread ++;
            InsertTailList(&g_ThreadListHead->ThreadListHead, &FoundThread->ThreadLink);
        }
        return g_ThreadListHead;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        if (g_ThreadListHead) {
            ExFreePoolWithTag(g_ThreadListHead, MEM_TAG);
            g_ThreadListHead = NULL;
        }
        return NULL;
    }
}
//////////////////////////////////////////////////////////////////////////

ULONG ExCopyThreadList2Buffer(PTHREAD_INFO ThreadInfo)
{
    PTHREAD_INFO tempThread;
    ULONG ReturnLength = 0;

    if (g_ThreadListHead == NULL)  return 0;

    while (!IsListEmpty(&g_ThreadListHead->ThreadListHead))
    {
        tempThread = (PTHREAD_INFO)RemoveHeadList(&g_ThreadListHead->ThreadListHead);
        RtlCopyMemory(ThreadInfo, tempThread, sizeof(THREAD_INFO));
        ExFreePoolWithTag(tempThread, MEM_TAG);
        ThreadInfo ++;
        ReturnLength ++;
    }

    ExFreePoolWithTag(g_ThreadListHead, MEM_TAG);
    g_ThreadListHead = NULL;
    return ReturnLength * sizeof(THREAD_INFO);
}

//////////////////////////////////////////////////////////////////////////

//
// 根据线程信息查找其所属进程信息
//
PEPROCESS ScPsGetThreadProcess(PETHREAD Thread)
{
    PKAPC_STATE ApcState = NULL;
    UCHAR ApcStateIndex;

    if (!MmIsAddressValid(Thread))  return NULL;

    __try {
        ApcStateIndex = *(PUCHAR)((ULONG)Thread + offset_Thread_ApcStateIndex);

        if (ApcStateIndex == 0) {

            ApcState = (PKAPC_STATE)((ULONG)Thread + offset_Thread_ApcState);
            return (PEPROCESS)ApcState->Process;

        } else if (ApcStateIndex == 1) {

            ApcState = (PKAPC_STATE)((ULONG)Thread + offset_Thread_SavedApcState);
            return (PEPROCESS)ApcState->Process;

        } else {

            return PsGetThreadProcess(Thread);
        }
    } 
    __except(EXCEPTION_EXECUTE_HANDLER) { return NULL; }
}

//////////////////////////////////////////////////////////////////////////

PEPROCESS ScPsGetSystemIdleProcess()
{
    PETHREAD IdleThread;
    
    //
    // see offset_kpcr_IdleProcess
    // This offset is no changed on xp and windows 7
    //
    __asm {
        mov eax, fs:[0x12c]
        mov IdleThread, eax
    }
    return ScPsGetThreadProcess(IdleThread);
}

//////////////////////////////////////////////////////////////////////////

ULONG 
ScPsGetCsrssProcessId()
/*++
    在vista中用ZwDuplicateObject复制csrss.exe中ALPC Port类型的句柄会返回C00000BB.
    现在采用变通的方法:用ZwQuerySystemInformation得到句柄表，再根据表中的ProcessId用
    sLookupProcessByProcessId得到peProcess,然后KeAttachProcess(peProcess)里，再用
    ObReferenceObjectByHandle得到obj这样就可以用ObQueryNameString得到句柄名称了 
--*/
{
    NTSTATUS ntStatus;
    PSYSTEM_HANDLE_INFORMATION Handles;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO HandleEntry;
    PVOID Buffer = NULL;
    ULONG BufferSize = 0x4000;
    ULONG ReturnLength;

    ULONG CsrssId = 0;
    OBJECT_ATTRIBUTES objAttributes;
    CLIENT_ID ClientId;
    HANDLE HandleProcess;
    HANDLE HandleObject;
    POBJECT_NAME_INFORMATION objName;
    UNICODE_STRING ApiPortName;
    ULONG i;

    objName = ExAllocatePoolWithTag(PagedPool, 0x400, MEM_TAG);
    RtlInitUnicodeString(&ApiPortName, L"\\Windows\\ApiPort");

_retry:
    Buffer = ExAllocatePoolWithTag(PagedPool, BufferSize, MEM_TAG);
    if (Buffer == NULL)  goto _theEnd;

    ntStatus = ZwQuerySystemInformation(SystemHandleInformation, 
                                        Buffer, 
                                        BufferSize, 
                                        &ReturnLength);

    if (ntStatus == STATUS_INFO_LENGTH_MISMATCH) {
        ExFreePool(Buffer);
        BufferSize = ReturnLength;
        goto _retry;
    }

    Handles = (PSYSTEM_HANDLE_INFORMATION)Buffer;
    HandleEntry = &(Handles->Handles[0]);

    for (i = 0; i < Handles->NumberOfHandles; i++, HandleEntry++)
    {
        if (HandleEntry->ObjectTypeIndex != 21)  continue;
        
        InitializeObjectAttributes(&objAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

        ClientId.UniqueProcess = (HANDLE)HandleEntry->UniqueProcessId;
        ClientId.UniqueThread  = 0;

        ntStatus = ZwOpenProcess(&HandleProcess, 
                                 PROCESS_DUP_HANDLE, 
                                 &objAttributes, 
                                 &ClientId );

        if (!NT_SUCCESS(ntStatus)) continue;
        
        ntStatus = ZwDuplicateObject(HandleProcess, 
                                 (HANDLE)HandleEntry->HandleValue, 
                                 NtCurrentProcess(), 
                                 &HandleObject, 
                                 0, 0, 
                                 DUPLICATE_SAME_ACCESS);

        if (NT_SUCCESS(ntStatus)) {
            ntStatus = ZwQueryObject(HandleObject, 
                                 ObjectNameInformation, 
                                 &objName, 
                                 0x400, NULL);

            if (NT_SUCCESS(ntStatus)) {
                if (wcsncmp(ApiPortName.Buffer, 
                            objName->Name.Buffer, 20) == 0 && 
                    objName->Name.Buffer != NULL) {
                    CsrssId = HandleEntry->UniqueProcessId;
                }
            }
            ZwClose(HandleObject); 
        }
        ZwClose(HandleProcess);
    }

    ExFreePool(Buffer);
_theEnd:
    ExFreePool(objName);
    return CsrssId;
}

//////////////////////////////////////////////////////////////////////////
NTSTATUS 
ScPsLookupProcessByName(PEPROCESS* TargetProcess, PWCH pszImageName)
{
    NTSTATUS ntStatus;
    PSYSTEM_PROCESS_INFORMATION ProcessEntry;
    ULONG BufferSize = 0x4000;
    ULONG ReturnLength;
    PVOID Buffer = NULL;
    UNICODE_STRING ImageName;
    OBJECT_ATTRIBUTES ObAttributes;
    CLIENT_ID ClientId;
    PEPROCESS FoundProcess;

    if (!MmIsAddressValid(TargetProcess))  return STATUS_ACCESS_DENIED;

_Retry:

    Buffer = ExAllocatePoolWithTag(PagedPool, BufferSize, MEM_TAG);
    if (Buffer == NULL)  return STATUS_UNSUCCESSFUL;

    ntStatus = ZwQuerySystemInformation(SystemProcessInformation, 
                                        Buffer, 
                                        BufferSize, 
                                        &ReturnLength);
    if (ntStatus == STATUS_INFO_LENGTH_MISMATCH) 
    {
        ExFreePool(Buffer);
        BufferSize = ReturnLength;
        goto _Retry;
    }

    RtlInitUnicodeString(&ImageName, pszImageName);

    if (!NT_SUCCESS(ntStatus))  return ntStatus;

    ProcessEntry = (PSYSTEM_PROCESS_INFORMATION)Buffer;
    while (ProcessEntry)
    {
        if (ProcessEntry->ImageName.Buffer == NULL)
        {
            ProcessEntry = (PSYSTEM_PROCESS_INFORMATION)
                ((ULONG)ProcessEntry + ProcessEntry->NextEntryDelta);
            continue;
        }

        if (RtlCompareUnicodeString(&ImageName, &ProcessEntry->ImageName, TRUE) == 0) 
        {
            KdPrint(("[SpsLookupProcessByName] %ws pid = %d", 
                    pszImageName, ProcessEntry->UniqueProcessId));

            ClientId.UniqueProcess = ProcessEntry->UniqueProcessId;
            ClientId.UniqueThread  = 0;

            InitializeObjectAttributes(&ObAttributes, 0, 0, 0, 0);
            ntStatus = PsLookupProcessByProcessId(ProcessEntry->UniqueProcessId,
                                                  &FoundProcess);
            if (NT_SUCCESS(ntStatus))  { TargetProcess[0] = FoundProcess; break; }
        }
        ProcessEntry = (PSYSTEM_PROCESS_INFORMATION)
            ((ULONG)ProcessEntry + ProcessEntry->NextEntryDelta);
    }
    
    ExFreePool(Buffer);
    return ntStatus;
}
//////////////////////////////////////////////////////////////////////////

VOID ListModuleThread(PVOID Context)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    ULONG StepAddress;
    ULONG Step2Address;
    ULONG BufferSize = 0x200;
    ULONG ReturnLength = 0;
    WCHAR LastImageName[260] = { 0 };
    HANDLE HandleProcess;
    PMEMORY_SECTION_NAME SectionName = NULL;
    MEMORY_BASIC_INFORMATION BasicInformation;
    PTHREAD_CONTEXT ThreadContext = Context;
    PMODULE_INFO FoundModule = NULL;
    pFnZwQueryVirtualMemory ZwQueryVirtualMemory = NULL;
    
    ZwQueryVirtualMemory = (pFnZwQueryVirtualMemory)
        KeServiceDescriptorTable.ServiceTableBase[ServiceId_NtQueryVirtualMemory];

    ntStatus = ObOpenObjectByPointer(ThreadContext->Process, OBJ_INHERIT, 
                                     NULL, 0, *PsProcessType, 
                                     ExGetPreviousMode(), &HandleProcess);
    if (!NT_SUCCESS(ntStatus)) {
        ExFreePoolWithTag(g_ModuleListHead, MEM_TAG);
        g_ModuleListHead = NULL;  goto _End;
    }

    SectionName = ExAllocatePoolWithTag(PagedPool, BufferSize, MEM_TAG);

    for (StepAddress = 0; StepAddress <= 0x7FFFFFFF; StepAddress += 0x10000)
    {
        ntStatus = ZwQueryVirtualMemory(HandleProcess,
                                        (PVOID)StepAddress, 
                                        MemoryBasicInformation,
                                        &BasicInformation, 
                                        sizeof(MEMORY_BASIC_INFORMATION), 
                                        &ReturnLength);

        if (!NT_SUCCESS(ntStatus) || BasicInformation.Type != SEC_IMAGE)  continue;
_Retry:        
        ntStatus = ZwQueryVirtualMemory(HandleProcess, 
                                        (PVOID)StepAddress, 
                                        MemorySectionName,                       
                                        SectionName, 
                                        BufferSize, 
                                        &ReturnLength);

        if (!NT_SUCCESS(ntStatus)) {
            if (ntStatus == STATUS_INFO_LENGTH_MISMATCH) {
                ExFreePoolWithTag(SectionName, MEM_TAG);
                SectionName = ExAllocatePoolWithTag(PagedPool, ReturnLength, MEM_TAG);
                goto _Retry;
            }
            continue;
        }
        __try {
            if (memcmp(LastImageName, SectionName->SectionFileName.Buffer, 
                       SectionName->SectionFileName.Length) &&
                SectionName->SectionFileName.Length < 520) {

                memcpy(LastImageName, SectionName->SectionFileName.Buffer,
                       SectionName->SectionFileName.Length);
                LastImageName[SectionName->SectionFileName.Length / 2] = L'\0';

                //
                // Step into and get the image size
                //
                for (Step2Address = StepAddress + BasicInformation.RegionSize;
                     Step2Address < 0x7FFFFFFF; 
                     Step2Address += BasicInformation.RegionSize) {

                    ntStatus = ZwQueryVirtualMemory(HandleProcess, 
                                                    (PVOID)Step2Address,
                                                    MemoryBasicInformation, 
                                                    &BasicInformation, 
                                                    sizeof(MEMORY_BASIC_INFORMATION), 
                                                    &ReturnLength);
                    if (NT_SUCCESS(ntStatus) && 
                        BasicInformation.Type != SEC_IMAGE)  break;
                }
                
                FoundModule = ExAllocatePoolWithTag(NonPagedPool, sizeof(MODULE_INFO), MEM_TAG);
                FoundModule->BaseAddress = StepAddress;
                FoundModule->ImageSize = Step2Address - StepAddress;
                RtlStringCbPrintfW(FoundModule->ImagePath, 520, L"%s", LastImageName);
                
                InsertTailList(&g_ModuleListHead->ModuleListHead, &FoundModule->ModuleLink);
                g_ModuleListHead->NumberOfModules ++;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) { continue; }    
    }
    ExFreePoolWithTag(SectionName, MEM_TAG);
    ObCloseHandle(HandleProcess, ExGetPreviousMode());
_End:
    KeSetEvent(&ThreadContext->SynEvent, IO_NO_INCREMENT, FALSE);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////
PMODULE_LIST_HEAD 
ScPsQueryProcessModuleList(PEPROCESS EProcess)
{
    HANDLE hThread;
    PTHREAD_CONTEXT ThreadContext = NULL;

    if (!MmIsAddressValid(EProcess))  return NULL;

    if (g_ModuleListHead)  return g_ModuleListHead;

    g_ModuleListHead = ExAllocatePoolWithTag(NonPagedPool, sizeof(MODULE_LIST_HEAD), MEM_TAG);
    g_ModuleListHead->NumberOfModules = 0;
    InitializeListHead(&g_ModuleListHead->ModuleListHead);

    ThreadContext = ExAllocatePoolWithTag(PagedPool, sizeof(THREAD_CONTEXT), MEM_TAG);
    KeInitializeEvent(&ThreadContext->SynEvent, SynchronizationEvent, FALSE);
    ThreadContext->Process = EProcess;

    PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, 0, NULL,
                         ListModuleThread, ThreadContext);
    ZwClose(hThread);
    KeWaitForSingleObject(&ThreadContext->SynEvent, Executive, KernelMode, FALSE, NULL);

    ExFreePoolWithTag(ThreadContext, MEM_TAG);
    return g_ModuleListHead;
}

//////////////////////////////////////////////////////////////////////////

ULONG ExCopyModuleList2Buffer(PMODULE_INFO ModuleInfo)
{
    PMODULE_INFO tempModule;
    ULONG ReturnLength = 0;

    if (g_ModuleListHead == NULL)  return 0;

    while (!IsListEmpty(&g_ModuleListHead->ModuleListHead))
    {
        tempModule = (PMODULE_INFO)RemoveHeadList(&g_ModuleListHead->ModuleListHead);
        RtlCopyMemory(ModuleInfo, tempModule, sizeof(MODULE_INFO));
        ExFreePoolWithTag(tempModule, MEM_TAG);
        ModuleInfo ++;
        ReturnLength ++;
    }

    ExFreePoolWithTag(g_ModuleListHead, MEM_TAG);
    g_ModuleListHead = NULL;
    return ReturnLength * sizeof(MODULE_INFO);
}

//////////////////////////////////////////////////////////////////////////
