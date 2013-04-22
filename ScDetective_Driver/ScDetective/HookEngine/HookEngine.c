
#include "HookEngine.h"

NTSTATUS OpZwClose(
    __in HANDLE Handle,
    __in PUCHAR OpCodeAddress
    )
{
    NTSTATUS ntStatus;
    pFnZwClose pfnZwClose = (pFnZwClose)OpCodeAddress;
    ntStatus = pfnZwClose(Handle);
    return ntStatus;
}

NTSTATUS fake_ZwClose (
    __in HANDLE Handle
    )
{
    ULONG FunctionId;
    FunctionId = ((ULONG)&fakeAddressTable.pFnZwClose - (ULONG)&fakeAddressTable) / sizeof(ULONG);
    return OpZwClose(Handle, OrigOpCode[FunctionId]);
}

VOID __fastcall fake_KiInsertQueueApc (
    __in PKAPC Apc,
    __in KPRIORITY Increment
    )
{
    ULONG NowThread;
    ULONG NowProcess;
    PUCHAR ProcessName;
    
    NowThread  = *(PULONG)((ULONG)Apc + 8);
    NowProcess = *(PULONG)((ULONG)NowThread + offset_Thread_ThreadsProcess );
    ProcessName = (PUCHAR)((ULONG)NowProcess + offset_Process_ImageFileName);

    if (strstr("ScDetective", (PCHAR)ProcessName) != NULL && Increment == 2)
    {
        return ;
    }
    g_OrigKiInsertQueueApc(Apc, Increment);
}

NTSTATUS fake_ObReferenceObjectByHandle(
    __in HANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_TYPE ObjectType,
    __in KPROCESSOR_MODE AccessMode,
    __out PVOID *Object,
    __out_opt POBJECT_HANDLE_INFORMATION HandleInformation
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PCHAR ImageFileName;

    ntStatus = ObReferenceObjectByHandle( Handle, 
                                          DesiredAccess, 
                                          ObjectType, 
                                          AccessMode, 
                                          Object, 
                                          HandleInformation);
    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }
    
    if (DesiredAccess == PROCESS_TERMINATE && ObjectType == *PsProcessType) 
    {
        ImageFileName = (PCHAR)((ULONG)(*Object) + offset_Process_ImageFileName);

        if (_stricmp(ImageFileName, "ScDetective.exe") == 0) 
        {
            KdPrint(("[fake_ObReferenceObjectByHandle] Refused Operate[PROCESS_TERMINATE] %s", ImageFileName));
            ObDereferenceObject(*Object);
            return STATUS_INVALID_HANDLE;
        }
    } 
    else if (DesiredAccess == PROCESS_DUP_HANDLE && ObjectType == *PsProcessType) 
    {
        ImageFileName = (PCHAR)((ULONG)(*Object) + offset_Process_ImageFileName);

        if (_stricmp(ImageFileName, "ScDetective.exe") == 0) 
        {
            KdPrint(("[fake_ObReferenceObjectByHandle] Refused Operate[PROCESS_DUP_HANDLE] %s", ImageFileName));
            ObDereferenceObject(*Object);
            return STATUS_INVALID_HANDLE;
        }
    }   
    return ntStatus;
}

BOOLEAN CheckAddresses(PADDRESS_TABLE TableEntry, ULONG NumberOfAddress)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    BOOLEAN bFlag = 0;
    PSYSTEM_MODULE_INFORMATION Modules;
    PSYSTEM_MODULE_INFORMATION_ENTRY ModuleInfo;
    PVOID Buffer = NULL;
    ULONG BufferSize = 0x2000;
    ULONG ReturnLength;
    ULONG i;
    ULONG Number = 0;
    PULONG Entry = (PULONG)TableEntry;

_Retry:
    Buffer = ExAllocatePoolWithTag(NonPagedPool, BufferSize, MEM_TAG);

    ntStatus = ZwQuerySystemInformation(SystemModuleInformation, 
                                        Buffer,
                                        BufferSize, 
                                        &ReturnLength);

    if (ntStatus == STATUS_INFO_LENGTH_MISMATCH) {
        BufferSize = ReturnLength;
        ExFreePoolWithTag(Buffer, MEM_TAG);
        goto _Retry;
    }

    if (NT_SUCCESS(ntStatus)) {
        Modules = (PSYSTEM_MODULE_INFORMATION)Buffer;
        ModuleInfo = &(Modules->Modules[0]);

        do {
            if (((ULONG)ModuleInfo->Base + ModuleInfo->Size) < (Entry + Number)[0])  break; 
            Number ++; 
        } while (Number < NumberOfAddress);

        bFlag = TRUE;
    }
    ExFreePoolWithTag(Buffer, MEM_TAG);
    return bFlag;
}

BOOLEAN InitAddress2Hook()
{
    ULONG ServiceId;
    UNICODE_STRING usFuncName;

    if (NumberOfHookedFunction < PREPARE_HOOK_NUMBER) 
    {
        RtlZeroMemory(HookFlags, PREPARE_HOOK_NUMBER);
        NumberOfHookedFunction = PREPARE_HOOK_NUMBER;
    }

    fakeAddressTable.pFnZwOpenKey               =   0;
    fakeAddressTable.pFnZwClose                 =   0; // (ULONG)fake_ZwClose;
    fakeAddressTable.pFnZwQueryValueKey         =   0;
    fakeAddressTable.pFnZwDeleteKey             =   0;
    fakeAddressTable.pFnZwSetValueKey           =   0;
    fakeAddressTable.pFnZwCreateKey             =   
    fakeAddressTable.pFnZwDeleteValueKey        = 
    fakeAddressTable.pFnZwEnumValueKey          =   
    fakeAddressTable.pFnZwRestoreKey            =   
    fakeAddressTable.pFnZwReplaceKey            =   0;
    fakeAddressTable.pFnZwTerminateProcess      =   (ULONG)fake_ObReferenceObjectByHandle;
    fakeAddressTable.pFnZwDuplicateObject       =   (ULONG)fake_ObReferenceObjectByHandle;
    fakeAddressTable.pFnZwSetSystemInformation  = 
    fakeAddressTable.pFnZwCreateThread          =  
    fakeAddressTable.pFnZwTerminateThread       =   0;
    // 替换 KeInsertQueneApc 中的 KiInsertQueneApc
    fakeAddressTable.pFnKeInsertQueneApc        =   (ULONG)fake_KiInsertQueueApc;
           
    OrigAddressTable.pFnZwOpenKey               =   SYSCALL_ADDRESS(ZwOpenKey);
    OrigAddressTable.pFnZwClose                 =   SYSCALL_ADDRESS(ZwClose);
    OrigAddressTable.pFnZwQueryValueKey         =   SYSCALL_ADDRESS(ZwQueryValueKey);
    OrigAddressTable.pFnZwDeleteKey             =   SYSCALL_ADDRESS(ZwDeleteKey);
    OrigAddressTable.pFnZwSetValueKey           =   SYSCALL_ADDRESS(ZwSetValueKey);
    OrigAddressTable.pFnZwCreateKey             =   SYSCALL_ADDRESS(ZwCreateKey);
    OrigAddressTable.pFnZwDeleteValueKey        =   SYSCALL_ADDRESS(ZwDeleteValueKey);
    OrigAddressTable.pFnZwEnumValueKey          =   SYSCALL_ADDRESS(ZwEnumerateValueKey);
    OrigAddressTable.pFnZwRestoreKey            =   SYSCALL_ADDRESS(ZwRestoreKey);
    OrigAddressTable.pFnZwReplaceKey            =   SYSCALL_ADDRESS(ZwReplaceKey);
    OrigAddressTable.pFnZwTerminateProcess      =   SYSCALL_ADDRESS(ZwTerminateProcess);
    OrigAddressTable.pFnZwDuplicateObject       =   SYSCALL_ADDRESS(ZwDuplicateObject);
    OrigAddressTable.pFnZwSetSystemInformation  =   SYSCALL_ADDRESS(ZwSetSystemInformation);

    RtlInitUnicodeString(&usFuncName, L"KeInsertQueueApc");
    OrigAddressTable.pFnKeInsertQueneApc        =   (ULONG)MmGetSystemRoutineAddress(&usFuncName);

    ServiceId = ServiceId_NtCreateThread;
    if (ServiceId) {
        OrigAddressTable.pFnZwCreateThread      =   KeServiceDescriptorTable.ServiceTableBase[ServiceId];
    } else {
        OrigAddressTable.pFnZwCreateThread      =   0;
    } 
    ServiceId = ServiceId_NtTerminateThread;
    if (ServiceId) {
        OrigAddressTable.pFnZwTerminateThread   =   KeServiceDescriptorTable.ServiceTableBase[ServiceId];
    } else {
        OrigAddressTable.pFnZwTerminateThread   =   0;
    }

    return CheckAddresses(&OrigAddressTable, PREPARE_HOOK_NUMBER);
}

BOOLEAN 
xchg_value_hook(ULONG OrigAddress, ULONG fakeAddress, PUCHAR OpCodeMoved, PUCHAR HookFlag, PULONG HookStartAddress, PULONG pcbCoverLength, ULONG Number)
{
    BOOLEAN result = FALSE;
    ULONG OpCodeSize = 0;
    ULONG Length = 0;
    BOOL bFlag = FALSE;
    PUCHAR cPtr;
    PUCHAR pOpCode;
    KIRQL OrigIrql;
    PMDL MdlFuncAddress = NULL;
    ULONG DeltaAddress, OrigDeltaAddress;

    ULONG id_zwtp, id_zwdo, id_kiqa;

    id_zwtp = ((ULONG)&fakeAddressTable.pFnZwTerminateProcess - (ULONG)&fakeAddressTable) / sizeof(ULONG);
    id_zwdo = ((ULONG)&fakeAddressTable.pFnZwDuplicateObject  - (ULONG)&fakeAddressTable) / sizeof(ULONG);
    id_kiqa = ((ULONG)&fakeAddressTable.pFnKeInsertQueneApc   - (ULONG)&fakeAddressTable) / sizeof(ULONG);
    

    if (OrigAddress && fakeAddress && OpCodeMoved && 
        HookFlag && HookStartAddress && pcbCoverLength )
    {
        if (HookFlag[0] == TRUE)  return FALSE;
        
        if (Number == id_zwtp || Number == id_zwdo) 
        {
            for (cPtr  = (PUCHAR)OrigAddress; 
                 cPtr <= (PUCHAR)(OrigAddress + PAGE_SIZE);
                 cPtr  = (PUCHAR)(cPtr + Length))
            {
                Length = SizeOfCode(cPtr, &pOpCode);
                if (Length == 0)  break;

                //
                // 此处的特征码是 : NtTerminateProcess 和 NtDuplicateObject 共用的
                //
                if (*(PUCHAR)cPtr == 0x6A && 
                    *(PULONG)(cPtr + 2) == 0xE80875FF) 
                {
                    HookStartAddress[0] = (ULONG)(cPtr + 6);  
                    OpCodeSize = 4;  bFlag = TRUE;  break;
                }
            }
        }
        
        if (Number == id_kiqa) 
        {
            for (cPtr  = (PUCHAR)OrigAddress; 
                 cPtr <= (PUCHAR)(OrigAddress + PAGE_SIZE);
                 cPtr  = (PUCHAR)(cPtr + Length))
            {
                Length = SizeOfCode(cPtr, &pOpCode);
                if (Length == 0)  break;
                
                //
                // 此处的特征码是寻找 KeInsertQueneApc 中的 KiInsertQueneApc
                // 此处没有考虑安全性，比如此时的 KiInsertQueneApc 已经被替换了，以后完善
                //
                if (*(PUCHAR)cPtr == 0xE8 && 
                    *(PUSHORT)(cPtr + 5) == 0xD88A) 
                {
                    HookStartAddress[0] = (ULONG)(cPtr + 1);  
                    g_OrigKiInsertQueueApc = (pFnKiInsertQueueApc)(*(PULONG)(cPtr + 1) + (ULONG)cPtr + 5);
                    OpCodeSize = 4;  bFlag = TRUE;  break;
                }
            }
        }
        
        if (bFlag == FALSE)  return FALSE;

        if (!ScmMapVirtualAddress((PVOID)OrigAddress, 0x400, &MdlFuncAddress)) return FALSE;

        WPOFF();
        OrigIrql = KeRaiseIrqlToDpcLevel();

        DeltaAddress = fakeAddress - (HookStartAddress[0] - 1) - 5;
        OrigDeltaAddress = InterlockedExchange((PVOID)HookStartAddress[0], DeltaAddress);

        KeLowerIrql(OrigIrql);
        WPON();
        if (OpCodeSize)  HookFlag[0] = TRUE;
        pcbCoverLength[0] = OpCodeSize;

        //
        // 以下判断可以保证 NtTerminateProcess 和 NtDuplicateObject 中的 
        // ObReferenceObjectByHandle 恢复是安全的
        //
        if (Number == id_zwdo || Number == id_zwtp)  
            OrigDeltaAddress = (ULONG)ObReferenceObjectByHandle - (HookStartAddress[0] - 1) - 5;

        RtlCopyMemory(OpCodeMoved, &OrigDeltaAddress, 4);

        ScmUnmapVirtualAddress(MdlFuncAddress);
        result = TRUE;
    }
    return result;
}

ULONG DynamicInlineHook(ULONG TargetAddress, ULONG fakeAddress, PUCHAR OpCodeMoved)
{
    ULONG Length = 0;
    ULONG Result = 0;
    PUCHAR cPtr;
    PUCHAR pOpCode;
    PUCHAR JumpCode;  

    JumpCode = ExAllocatePoolWithTag(NonPagedPool, 16, MEM_TAG);
    JumpCode[0] = 0xE9;

    cPtr = (PUCHAR)TargetAddress;

    while (Length < 5) {
        Length += SizeOfCode(cPtr, &pOpCode);
        cPtr = (PUCHAR)(TargetAddress + Length);
    }

    RtlCopyMemory(OpCodeMoved, (PVOID)TargetAddress, Length);
    ((PULONG)(JumpCode + 1))[0] = (TargetAddress + Length) - ((ULONG)OpCodeMoved + Length) - 5;
    RtlCopyMemory(OpCodeMoved + Length, JumpCode, 5);

    ((PULONG)(JumpCode + 1))[0] = fakeAddress - TargetAddress - 5;

    if (ScHeSafeInlineHook((PVOID)TargetAddress, JumpCode, 5))  Result = Length;
    
    ExFreePoolWithTag(JumpCode, MEM_TAG);
    return Result;
}

BOOLEAN  
OpInlineHook(ULONG OrigAddress, ULONG fakeAddress, PUCHAR OpCodeMoved, 
             PUCHAR HookFlag, PULONG HookStartAddress, PULONG pcbCoverLength)
{
    BOOLEAN result = FALSE;
    ULONG OpCodeSize = 0;

    if (OrigAddress && fakeAddress && OpCodeMoved && 
        HookFlag && HookStartAddress && pcbCoverLength) 
    {
        if (HookFlag[0] == TRUE)  return FALSE;

        OpCodeSize = DynamicInlineHook(OrigAddress, fakeAddress, OpCodeMoved);

        if (OpCodeSize)  HookFlag[0] = TRUE;

        pcbCoverLength[0] = OpCodeSize;
        HookStartAddress[0] = OrigAddress;
        result = TRUE;
    }
    return result;
}

BOOLEAN ScHeInlineHookEngine(ULONG FunctionAddress, ULONG Id)
{
    BOOLEAN result = 0;
    ULONG fakeAddress;
    ULONG id_zwtp, id_zwdo, id_kiqa;

    fakeAddress = *((PULONG)&fakeAddressTable + Id);

    //
    // Id = 5 对应的是 ZwCreateKey
    //
    if (Id == 5)  return result;

    //
    // 如果是 NtTerminateProcess 或者 NtDuplicateObject 
    // 则特殊处理其调用的 ObReferenceObjectByHandle 函数
    //
    id_zwtp = ((ULONG)&fakeAddressTable.pFnZwTerminateProcess - (ULONG)&fakeAddressTable) / sizeof(ULONG);
    id_zwdo = ((ULONG)&fakeAddressTable.pFnZwDuplicateObject  - (ULONG)&fakeAddressTable) / sizeof(ULONG);

    // 
    // 替换 KeInsertQueneApc 中的 KiInsertQueneApc
    //
    id_kiqa = ((ULONG)&fakeAddressTable.pFnKeInsertQueneApc - (ULONG)&fakeAddressTable) / sizeof(ULONG);

    if (Id == id_zwtp || Id == id_zwdo || Id == id_kiqa) {

        return xchg_value_hook(FunctionAddress, fakeAddress, OrigOpCode[Id], 
                    &HookFlags[Id], &CoverStartAddress[Id], &CoverLength[Id], Id);
    }

    result = OpInlineHook(FunctionAddress, fakeAddress, OrigOpCode[Id], 
                    &HookFlags[Id],  &CoverStartAddress[Id], &CoverLength[Id]);

    return result;
}

//////////////////////////////////////////////////////////////////////////

VOID InitilizeHook()
{
    ULONG Number = 0;
    ULONG FuncAddress;

    if (bAlreadyHooked == TRUE)  return ;
    
    memset(HookFlags, 0, PREPARE_HOOK_NUMBER);
    memset(CoverLength, 0, PREPARE_HOOK_NUMBER * 4);
    memset(&OrigAddressTable, 0, sizeof(ADDRESS_TABLE));
    memset(&fakeAddressTable, 0, sizeof(ADDRESS_TABLE));

    if (InitAddress2Hook()) {

        do {
            FuncAddress = ((PULONG)&OrigAddressTable + Number)[0];
            ScHeInlineHookEngine(FuncAddress, Number);
            Number ++;
        } while (Number < PREPARE_HOOK_NUMBER);

        bAlreadyHooked ++;
        // 中间还有个function, 通过 HookFlags 判断是否通过解析 PE(ntoskrnl) 文件解析 ssdt 函数地址
        // 现在先不急着考虑，暂定ssdt表中的函数没有被更改
    }
}

BOOL 
OpUnInlineHook(ULONG OrigAddress, ULONG HookStartAddress, PUCHAR OpCodeMoved, PUCHAR HookFlag, ULONG cbCoverLength)
{
    BOOL result = FALSE;
    ULONG OpCodeSize = 0;


    if (OrigAddress && HookStartAddress && OpCodeMoved && 
        HookFlag && cbCoverLength) 
    {
        if (HookFlag[0] == FALSE)  return TRUE;
        
        result = ScHeSafeInlineHook((PVOID)HookStartAddress, OpCodeMoved, cbCoverLength);

        if (result) 
            HookFlag[0] = FALSE;
        else 
            HookFlag[0] = TRUE;
    }

    return result;
}

BOOL ScHeUnInlineHookEngine(ULONG FunctionAddress, ULONG Id)
{
    BOOL result = FALSE;

    //
    // Id = 5 对应的是 ZwCreateKey
    //
    if (Id == 5)  return result;

    result = OpUnInlineHook(FunctionAddress, CoverStartAddress[Id], 
                        OrigOpCode[Id], &HookFlags[Id], CoverLength[Id]);
                                
    return result;
}

VOID UnInlineHookNativeApi()
{
    ULONG Number = 0;
    ULONG FuncAddress;

    if (bAlreadyHooked) {

        do {
            FuncAddress = ((PULONG)&OrigAddressTable + Number)[0];
            ScHeUnInlineHookEngine(FuncAddress, Number);
            Number ++;
        } while (Number < PREPARE_HOOK_NUMBER);

        bAlreadyHooked --;
    }
}

//////////////////////////////////////////////////////////////////////////

VOID 
OpSafeInlineHook(PVOID TargetAddress, PVOID ReadyOpCode, ULONG OpCodeLength)
{
    PMDL MdlFuncAddress;

    ASSERT(TargetAddress && ReadyOpCode && OpCodeLength);

    if (ScmMapVirtualAddress(TargetAddress, 0x400, &MdlFuncAddress)) 
    {
        WPOFF();
        RtlCopyMemory(TargetAddress, ReadyOpCode, OpCodeLength);
        WPON();
        ScmUnmapVirtualAddress(MdlFuncAddress);
    }
}

VOID SafeHookDpcRoutine (
    __in struct _KDPC *Dpc,
    __in_opt PDPC_CONTEXT DeferredContext,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
    )
{
    InterlockedIncrement(&DeferredContext->LockedProcessors);
    do {
        __asm   pause;
    } while (DeferredContext->ReleaseFlag == FALSE);
    InterlockedDecrement(&DeferredContext->LockedProcessors);
}

BOOL ScHeSafeInlineHook(PVOID TargetAddress, PVOID ReadyOpCode, ULONG OpCodeLength)
{
    BOOL result = FALSE;
    DPC_CONTEXT DpcContext;
    KAFFINITY OrigAffinity;
    UNICODE_STRING NameString;
    CCHAR CurrentProcessor;
    CCHAR Processor;
    PKDPC Dpc;
    ULONG i;
    KIRQL OrigIrql;
    pFnKeSetAffinityThread KeSetAffinityThread = NULL;
    
    RtlInitUnicodeString(&NameString, L"KeSetAffinityThread");
    KeSetAffinityThread = (pFnKeSetAffinityThread)MmGetSystemRoutineAddress(&NameString);

    OrigAffinity = KeSetAffinityThread(KeGetCurrentThread(), 1); 
    OrigIrql = KeRaiseIrqlToDpcLevel();

    if (KeNumberProcessors > 1) {

        CurrentProcessor = (CCHAR)KeGetCurrentProcessorNumber();
        DpcContext.Dpcs = ExAllocatePoolWithTag(NonPagedPool, KeNumberProcessors * sizeof(KDPC), MEM_TAG);
        DpcContext.LockedProcessors = 1;
        DpcContext.ReleaseFlag = FALSE;

        for (Processor = 0; Processor < KeNumberProcessors; Processor++)
        {
            if (Processor == CurrentProcessor)  continue;
            Dpc = &DpcContext.Dpcs[Processor];
            KeInitializeDpc(Dpc, SafeHookDpcRoutine, &DpcContext);
            KeSetTargetProcessorDpc(Dpc, Processor);
            KeInsertQueueDpc(Dpc, NULL, NULL);
        }

        for (i = 0; i < 0x800000; i++) {
            __asm   pause;
            if (DpcContext.LockedProcessors == (ULONG)KeNumberProcessors) break;
        }
        
        if (DpcContext.LockedProcessors != (ULONG)KeNumberProcessors) {
            KdPrint(("[ScSafeInlineHook] Failed to insert dpc to other processors"));
            DpcContext.ReleaseFlag = TRUE;
            for (Processor = 0; Processor < KeNumberProcessors; Processor++) 
            {
                if (Processor != CurrentProcessor) {
                    KeRemoveQueueDpc(&DpcContext.Dpcs[Processor]);
                }
            }
        } else {
            KdPrint(("[ScSafeInlineHook] Insert dpc succeed, now start inline hook"));
            OpSafeInlineHook(TargetAddress, ReadyOpCode, OpCodeLength);
            result = TRUE;
            DpcContext.ReleaseFlag = TRUE;  
        }
        do {
            __asm   pause;
        } while (DpcContext.LockedProcessors != 1);

        ExFreePoolWithTag(DpcContext.Dpcs, MEM_TAG);

    } else {

        OpSafeInlineHook(TargetAddress, ReadyOpCode, OpCodeLength);
        result = TRUE;
    }
    KeLowerIrql(OrigIrql);
    KeSetAffinityThread(KeGetCurrentThread(), OrigAffinity); 
    return result;
}

//////////////////////////////////////////////////////////////////////////
