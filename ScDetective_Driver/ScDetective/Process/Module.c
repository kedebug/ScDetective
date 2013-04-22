
#include "module.h"

//////////////////////////////////////////////////////////////////////////

NTSTATUS SobGetObjectInformation(PVOID ActiveObject, POBJECT_CLASS ObjectClass)
{
    ULONG NowAddress;
    ULONG ReturnValue;
    ULONG ObjectType;
    ULONG MagicNumber;
    PVOID Object = NULL;
    
    if (ActiveObject == NULL || ObjectClass == NULL)  return STATUS_ACCESS_DENIED;

    ObjectType = ScObGetObjectType(ActiveObject);

    ObjectClass->Key = *(PULONG)(ObjectType + offset_ObjectType_Key) | 0x80000000;
    ObjectClass->NumberOfObject = *(PULONG)(ObjectType + offset_ObjectType_TotalNumberOfObjects);
    
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

            if (*((PULONG)NowAddress) != ObjectClass->Key)  continue;

            for (MagicNumber = 0; MagicNumber < 0x150; MagicNumber += 4)
            {
                Object = (PVOID)(NowAddress + MagicNumber);

                if (Object == ActiveObject) {

                    ObjectClass->MagicNumber = MagicNumber;
                    ObjectClass->Type = *((PUSHORT)Object);
                    ObjectClass->Size = *((PUSHORT)Object + 1);

                    return STATUS_SUCCESS;  
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) { continue; }
    }
    return STATUS_UNSUCCESSFUL;
}

//////////////////////////////////////////////////////////////////////////

VOID QueryWorkThread(PVOID ThreadContext)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PDRIVER_CONTEXT Context = ThreadContext;
    static OBJECT_CLASS ObjectClass;
    static ULONG Number = 1;
    ULONG NowAddress;
    PDRIVER_OBJECT Object = NULL;
    PDRIVER_INFO FoundDriver;
    PVOID DriverSection = NULL;
    ULONG i;
    ULONG ReturnValue;
    PDRIVER_INFO ReadyDriver = NULL;
    BOOLEAN LableFind = FALSE;
    PLIST_ENTRY p;
    PSYSTEM_MODULE_INFORMATION Modules;
    PSYSTEM_MODULE_INFORMATION_ENTRY ModuleInfo;
    PVOID Buffer = NULL;
    ULONG BufferSize = 0x2000;
    ULONG ReturnLength;

    //////////////////////////////////////////////////////////////////////////

_Retry:
    Buffer = ExAllocatePoolWithTag(NonPagedPool, BufferSize, MEM_TAG);

    ntStatus = ZwQuerySystemInformation(SystemModuleInformation, Buffer,
                                        BufferSize, &ReturnLength);
    if (ntStatus == STATUS_INFO_LENGTH_MISMATCH) {
        BufferSize = ReturnLength;
        ExFreePoolWithTag(Buffer, MEM_TAG);
        goto _Retry;
    }
    if (NT_SUCCESS(ntStatus)) {
        Modules = (PSYSTEM_MODULE_INFORMATION)Buffer;
        ModuleInfo = &(Modules->Modules[0]);
        for (i = 0; i < Modules->NumberOfModules; i++, ModuleInfo++)
        {
            if ((ULONG)ModuleInfo->Base < 0x80000000)  continue;

            FoundDriver = ExAllocatePoolWithTag(NonPagedPool, sizeof(DRIVER_INFO), MEM_TAG);
            RtlZeroMemory(FoundDriver, sizeof(DRIVER_INFO));

            FoundDriver->ImageBase = (ULONG)ModuleInfo->Base;
            FoundDriver->DriverSize = ModuleInfo->Size;
            FoundDriver->bHidden = FALSE;

            if (ModuleInfo->OffsetToFileName != 0)
                 RtlStringCbPrintfW(FoundDriver->ImagePath, 520, 
                                    L"%S", ModuleInfo->FullPathName);
            else RtlStringCbPrintfW(FoundDriver->ImagePath, 520, 
                                    L"\\SystemRoot\\System32\\Drivers\\%S", 
                                    ModuleInfo->FullPathName);

            InsertTailList(&g_DriverListHead->DriverListHead, &FoundDriver->DriverLink);
            g_DriverListHead->NumberOfDrivers ++;
        }
    } 
    ExFreePoolWithTag(Buffer, MEM_TAG);

    //////////////////////////////////////////////////////////////////////////

    if (Number == 1) {
        ntStatus = SobGetObjectInformation(Context->DriverObject, &ObjectClass);
        if (!NT_SUCCESS(ntStatus)) {
            ExFreePoolWithTag(g_DriverListHead, MEM_TAG);
            g_DriverListHead = NULL;  return ;
        }   Number --; 
    }

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

            if (((PULONG)NowAddress)[0] != ObjectClass.Key)     continue;

            Object = (PDRIVER_OBJECT)(NowAddress + ObjectClass.MagicNumber);

            if (!MmIsAddressValid(Object))                      continue;

            if (((PUSHORT)Object)[0] != ObjectClass.Type)       continue;
            if (((PUSHORT)Object + 1)[0] != ObjectClass.Size)   continue;

            if (!MmIsAddressValid(Object->DriverSection))       continue;

            DriverSection = Object->DriverSection;

            if (((PULONG)((ULONG)DriverSection + offset_LdrData_DLLBase))[0] != 
                (ULONG)(Object->DriverStart))  { continue; }

            FoundDriver = ExAllocatePoolWithTag(NonPagedPool, sizeof(DRIVER_INFO), MEM_TAG);
            RtlZeroMemory(FoundDriver, sizeof(DRIVER_INFO));

            FoundDriver->DriverObject = (ULONG)Object;
            FoundDriver->ImageBase = (ULONG)Object->DriverStart;

            if (FoundDriver->ImageBase < 0x80000000) {
                ExFreePoolWithTag(FoundDriver, MEM_TAG); continue;
            }

            FoundDriver->DriverSize = *(PULONG)((ULONG)DriverSection + offset_LdrData_SizeOfImage);

            RtlCopyMemory(FoundDriver->ServiceName, 
                          Object->DriverExtension->ServiceKeyName.Buffer, 
                          Object->DriverExtension->ServiceKeyName.Length);

            for (p = g_DriverListHead->DriverListHead.Flink; 
                 p != &g_DriverListHead->DriverListHead; 
                 p = p->Flink) {

                ReadyDriver = CONTAINING_RECORD(p, DRIVER_INFO, DriverLink);
                if (FoundDriver->ImageBase == ReadyDriver->ImageBase) {

                    ReadyDriver->DriverObject = FoundDriver->DriverObject;
                    RtlCopyMemory(ReadyDriver->ServiceName, FoundDriver->ServiceName, 64 * 2);
                    ExFreePoolWithTag(FoundDriver, MEM_TAG);

                    LableFind = TRUE;   break;
                }
            }

            if (LableFind == TRUE) {
                LableFind = FALSE;
            } else {
                FoundDriver->bHidden = TRUE;
                RtlStringCbPrintfW(FoundDriver->ImagePath, 520, L"%wZ", 
                                  (ULONG)DriverSection + offset_LdrData_FullDllName);

                InsertTailList(&g_DriverListHead->DriverListHead, &FoundDriver->DriverLink);
                g_DriverListHead->NumberOfDrivers ++;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) { continue; } 
    }
    KeSetEvent(Context->kEvent, IO_NO_INCREMENT, FALSE);
    ExFreePoolWithTag(Context, MEM_TAG);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////
PDRIVER_LIST_HEAD 
ScObQueryDriverObject(PDRIVER_OBJECT DriverObject, PKEVENT UserEvent)
{
    PDRIVER_CONTEXT Context;
    HANDLE hThread;

    if (g_DriverListHead)  return g_DriverListHead;

    if (DriverObject == NULL || UserEvent == NULL)  return NULL;

    g_DriverListHead = ExAllocatePoolWithTag(NonPagedPool, sizeof(DRIVER_LIST_HEAD), MEM_TAG);
    InitializeListHead(&g_DriverListHead->DriverListHead);
    g_DriverListHead->NumberOfDrivers = 0;

    Context = ExAllocatePoolWithTag(NonPagedPool, sizeof(DRIVER_CONTEXT), MEM_TAG);
    Context->DriverObject = DriverObject;
    Context->kEvent = UserEvent;

    PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, 
                         NtCurrentProcess(), NULL, QueryWorkThread, Context);
    ZwClose(hThread);
    return NULL;
}

//////////////////////////////////////////////////////////////////////////

ULONG ExCopyDriverList2Buffer(PDRIVER_INFO DriverInfo)
{
    PDRIVER_INFO tempDriver = NULL;
    ULONG ReturnLength = 0;

    if (g_DriverListHead == NULL)  return 0;

    while (!IsListEmpty(&g_DriverListHead->DriverListHead))
    {
        tempDriver = (PDRIVER_INFO)RemoveHeadList(&g_DriverListHead->DriverListHead);
        RtlCopyMemory(DriverInfo, tempDriver, sizeof(DRIVER_INFO));
        ExFreePoolWithTag(tempDriver, MEM_TAG);
        DriverInfo ++;
        ReturnLength ++;
    }

    ExFreePoolWithTag(g_DriverListHead, MEM_TAG);
    g_DriverListHead = NULL;
    return ReturnLength * sizeof(DRIVER_INFO);
}

//////////////////////////////////////////////////////////////////////////