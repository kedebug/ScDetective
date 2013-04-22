///////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2010 - kedebug
///
/// Original filename: ScDetective.cpp
/// Project          : ScDetective
/// Date of creation : 2010-10-16
/// Author(s)        : kedebug(SJTU)
///
/// Purpose          : Only for study
///
/// Revisions:
///  0000 [2010-10-16] Initial revision.
///
///////////////////////////////////////////////////////////////////////////////

// $Id$


#include "ScDetective.h"

#include "System/Initialize.c"
#include "ssdt/ssdt.c"
#include "ssdt/ssdt_shadow.c"
#include "LDasm/LDasm.c"
#include "Process/Process.c"
#include "Process/module.c"
#include "File/File.c"
#include "Memory/memory.c"
#include "HookEngine/HookEngine.c"
#include "Protect/ScProtect.c"

#ifdef __cplusplus
namespace { // anonymous namespace to limit the scope of this global variable!
#endif
PDRIVER_OBJECT pdoGlobalDrvObj = 0;
#ifdef __cplusplus
}; // anonymous namespace
#endif

NTSTATUS ScDetective_DispatchCreate(
    IN PDEVICE_OBJECT		DeviceObject,
    IN PIRP					Irp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS ScDetective_DispatchClose(
    IN PDEVICE_OBJECT		DeviceObject,
    IN PIRP					Irp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LARGE_INTEGER interval;
    HANDLE hThread;
    
    KdPrint(("[ScDetective_DispatchClose] Enter DispatchClose..."));

    UnInlineHookNativeApi();

    interval.QuadPart = - 4 * 1000 * 100;     // 40ms, relative
    KeDelayExecutionThread(KernelMode, FALSE, &interval);

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    KdPrint(("[ScDetective_DispatchClose] Leave DispatchClose..."));
    return status;
}

NTSTATUS ScDetective_DispatchDeviceControl(
    IN PDEVICE_OBJECT		DeviceObject,
    IN PIRP					Irp
    )
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PVOID       InputBuffer     = NULL;
    PVOID       OutputBuffer    = NULL;
    ULONG       cbInputBuffer   = 0;
    ULONG       cbOutputBuffer  = 0;
    PIO_STACK_LOCATION irpSp    = NULL;
    
    __try {
        irpSp = IoGetCurrentIrpStackLocation(Irp);

        InputBuffer     = Irp->AssociatedIrp.SystemBuffer;
        cbInputBuffer   = irpSp->Parameters.DeviceIoControl.InputBufferLength;
        OutputBuffer    = Irp->AssociatedIrp.SystemBuffer;
        cbOutputBuffer  = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

        switch(irpSp->Parameters.DeviceIoControl.IoControlCode)
        {
        case IOCTL_DUMP_KERNEL_MEMORY: 
            {
                PVOID DumpAddress;
                PMDL MdlCreate;

                if (cbInputBuffer == sizeof(ULONG)) {
                    DumpAddress = (PVOID)((PULONG)InputBuffer)[0];
                    if (!MmIsAddressValid(DumpAddress)) {
                        ntStatus = STATUS_INVALID_ADDRESS;
                        break;
                    } else {
                        ScmMapVirtualAddress(DumpAddress, cbOutputBuffer, &MdlCreate);
                        RtlCopyMemory(OutputBuffer, DumpAddress, cbOutputBuffer);
                        ScmUnmapVirtualAddress(MdlCreate);
                        Irp->IoStatus.Information = cbOutputBuffer;  break; 
                    }
                } else {
                    ntStatus = STATUS_BUFFER_TOO_SMALL;  break;
                }
            }
        //////////////////////////////////////////////////////////////////////////
        case IOCTL_GET_SSDT:    // ��ȡ ssdt
            {         
                ULONG NeedLen = 0;
                ULONG Number = GetSsdtServiceNumber();

                NeedLen = Number * sizeof(SSDT_ADDRESS);
                if (cbOutputBuffer < NeedLen) {
                    if (cbOutputBuffer == sizeof(ULONG)) {
                        ((PULONG)OutputBuffer)[0] = NeedLen;
                        Irp->IoStatus.Information = sizeof(ULONG);
                        break;
                    }
                    ntStatus = STATUS_BUFFER_TOO_SMALL;  break;
                }
                Number = GetSsdtCurrentAddresses((PSSDT_ADDRESS)OutputBuffer, &NeedLen);
                if (Number == 0)  ntStatus = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = Number * sizeof(SSDT_ADDRESS);
                break;
            } 
        //////////////////////////////////////////////////////////////////////////
        case IOCTL_UNHOOK_SSDT:          // �ָ� ssdt
            {
                PSSDT_ADDRESS SsdtOrig = (PSSDT_ADDRESS)InputBuffer;

                if (cbInputBuffer < sizeof(SSDT_ADDRESS) || 
                    InputBuffer == NULL) {
                    KdPrint(("���뻺���������뻺����������Ч"));
                    ntStatus = STATUS_UNSUCCESSFUL;
                    break;
                }
                KdPrint(("Ҫ�ָ��ķ����ţ�%d ԭʼ��ַ��0x%X", 
                    SsdtOrig->nIndex, SsdtOrig->FunAddress));

                if (!UnHookSsdtItem(SsdtOrig)) {
                    KdPrint(("�ָ�ʧ��"));
                    ntStatus = STATUS_UNSUCCESSFUL;
                }
                break;
            } 
        //////////////////////////////////////////////////////////////////////////
        case IOCTL_GET_SSDTSHADOW:
            {
                ULONG Number = GetShadowSsdtServiceNumber();
                ULONG NeedLen = 0;
                
                NeedLen = Number * sizeof(SSDT_ADDRESS);
                if (cbOutputBuffer < NeedLen) {
                    if (cbOutputBuffer == sizeof(ULONG)) {
                        ((PULONG)OutputBuffer)[0] = NeedLen;
                        Irp->IoStatus.Information = sizeof(ULONG);
                        break;
                    }
                    ntStatus = STATUS_BUFFER_TOO_SMALL;  break;
                }
                Number = GetShadowSsdtCurrentAddresses((PSSDT_ADDRESS)OutputBuffer, &NeedLen);

                if (Number == 0)  ntStatus = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = Number * sizeof(SSDT_ADDRESS);
                break;
            }
        //////////////////////////////////////////////////////////////////////////
        case IOCTL_UNHOOK_SSDTSHADOW:
            {
                PSSDT_ADDRESS ShadowSsdtOrig = (PSSDT_ADDRESS)InputBuffer;

                if (cbInputBuffer < sizeof(SSDT_ADDRESS) || 
                    InputBuffer == NULL) {
                    KdPrint(("���뻺���������뻺����������Ч"));
                    ntStatus = STATUS_UNSUCCESSFUL;  break;
                }
                KdPrint(("Ҫ�ָ��ķ����ţ�%d ԭʼ��ַ��0x%X", 
                    ShadowSsdtOrig->nIndex, ShadowSsdtOrig->FunAddress));

                if (!UnHookShadowSsdtItem(ShadowSsdtOrig, g_CsrssProcess)) {
                    ntStatus = STATUS_UNSUCCESSFUL;
                }
                break;
            }
        //////////////////////////////////////////////////////////////////////////
        case IOCTL_GET_PROCESSES:  
            {
                PPROCESS_LIST_HEAD ProcessHead;
                ULONG NeedLen;
                ULONG ReturnLength;

                ProcessHead = ScPsQuerySystemProcessList();
                NeedLen = ProcessHead->NumberOfProcesses * sizeof(PROCESS_INFO);

                if (cbOutputBuffer < NeedLen) {
                    if (cbOutputBuffer == sizeof(ULONG)) {
                        ((PULONG)OutputBuffer)[0] = NeedLen;
                        Irp->IoStatus.Information = sizeof(ULONG);
                        break;
                    }
                    ntStatus = STATUS_BUFFER_TOO_SMALL; break;
                }
                ReturnLength = ExCopyProcessList2Buffer((PPROCESS_INFO)OutputBuffer);
                if (ReturnLength == 0)  ntStatus = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = ReturnLength;
                break;
            }
        //////////////////////////////////////////////////////////////////////////
        case IOCTL_GET_PROCESS_IMAGE_PATH:
            {
                PEPROCESS Process = NULL;
                PUNICODE_STRING NameString;
                ULONG BufferSize;

                if (cbInputBuffer == sizeof(ULONG)) {
                    Process = ((PEPROCESS*)InputBuffer)[0];
                    if (Process == NULL) {
                        ntStatus = STATUS_ACCESS_DENIED; break;
                    }
                } else {
                    ntStatus = STATUS_BUFFER_TOO_SMALL;
                    break;
                }
                if (Process == g_SystemProcess) {
                    if (cbOutputBuffer > sizeof(L"System")) {
                        RtlCopyMemory(OutputBuffer, L"System", sizeof(L"System"));
                        Irp->IoStatus.Information = sizeof(L"System");
                        break; 
                    }
                } else if (Process == g_IdleProcess) {
                    if (cbOutputBuffer > sizeof(L"Idle")) {
                        RtlCopyMemory(OutputBuffer, L"Idle", sizeof(L"Idle"));
                        Irp->IoStatus.Information = sizeof(L"Idle");
                        break; 
                    }
                }
                if (cbOutputBuffer < 520) {
                    ntStatus = STATUS_BUFFER_TOO_SMALL;
                    break;
                }
                BufferSize = cbOutputBuffer + sizeof(UNICODE_STRING);
                NameString = ExAllocatePoolWithTag(NonPagedPool, BufferSize, MEM_TAG);
                NameString->Buffer = (PWCH)((ULONG)NameString + 8);
                NameString->Length = 0;
                NameString->MaximumLength = (USHORT)cbOutputBuffer;

                ntStatus = ScPsGetProcessImagePath(Process, NameString);
                if (NT_SUCCESS(ntStatus)) {
                    RtlCopyMemory(OutputBuffer, NameString->Buffer, NameString->Length);
                }
                Irp->IoStatus.Information = NameString->Length;
                ExFreePoolWithTag(NameString, MEM_TAG);
                break;
            }
        //////////////////////////////////////////////////////////////////////////
        case IOCTL_GET_PROCESS_THREADS:
            {            
                PTHREAD_LIST_HEAD ThreadHead = NULL;
                PEPROCESS EProcess = NULL;
                ULONG NeedLen = 0;
                ULONG ReturnLength = 0;
                
                if (cbInputBuffer == sizeof(ULONG)) {
                    EProcess = ((PEPROCESS*)InputBuffer)[0];
                } else {
                    ntStatus = STATUS_BUFFER_TOO_SMALL;
                    break;
                }
                if (EProcess == g_IdleProcess) {
                    if (cbOutputBuffer == sizeof(ULONG)) {
                        ((PULONG)OutputBuffer)[0] = NeedLen;  
                        Irp->IoStatus.Information = sizeof(ULONG);
                        break; 
                    }
                }
                ThreadHead = ScPsQueryProcessThreadList(EProcess);
                if (ThreadHead == NULL) {
                    ntStatus = STATUS_UNSUCCESSFUL;
                    break;
                }
                NeedLen = ThreadHead->NumberOfThread * sizeof(THREAD_INFO);
 
                if (cbOutputBuffer < NeedLen) {
                    if (cbOutputBuffer == sizeof(ULONG)) {
                        ((PULONG)OutputBuffer)[0] = NeedLen;
                        Irp->IoStatus.Information = sizeof(ULONG);
                        break;
                    }
                    ntStatus = STATUS_BUFFER_TOO_SMALL;  break;
                }
                ReturnLength = ExCopyThreadList2Buffer((PTHREAD_INFO)OutputBuffer);
                if (ReturnLength == 0)  ntStatus = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = ReturnLength;
                break;
            }
        //////////////////////////////////////////////////////////////////////////
        case IOCTL_GET_PROCESS_MODULES:
            {
                PMODULE_LIST_HEAD ModuleHead = NULL;
                PEPROCESS EProcess = NULL;
                ULONG NeedLen = 0;
                ULONG ReturnLength = 0;

                if (cbInputBuffer == sizeof(ULONG)) {
                    EProcess = ((PEPROCESS*)InputBuffer)[0];
                } else {
                    ntStatus = STATUS_BUFFER_TOO_SMALL;  break;
                }

                if (EProcess == g_IdleProcess) {
                    if (cbOutputBuffer = sizeof(ULONG)) {
                        ((PULONG)OutputBuffer)[0] = NeedLen;
                        Irp->IoStatus.Information = sizeof(ULONG);
                        break; 
                    }
                }
                ModuleHead = ScPsQueryProcessModuleList(EProcess);
                if (ModuleHead == NULL) {
                    ntStatus = STATUS_UNSUCCESSFUL;  break;
                }
                NeedLen = ModuleHead->NumberOfModules * sizeof(MODULE_INFO);

                if (cbOutputBuffer < NeedLen) {
                    if (cbOutputBuffer == sizeof(ULONG)) {
                        ((PULONG)OutputBuffer)[0] = NeedLen;
                        Irp->IoStatus.Information = sizeof(ULONG);
                        break;
                    }
                    ntStatus = STATUS_BUFFER_TOO_SMALL;  break;
                }
                ReturnLength = ExCopyModuleList2Buffer((PMODULE_INFO)OutputBuffer);
                if (ReturnLength == 0)  ntStatus = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = ReturnLength;
                break;
            }
        //////////////////////////////////////////////////////////////////////////
        case IOCTL_GET_DRIVER_OBJECT:
            {
                PDRIVER_LIST_HEAD DriverHead = NULL;
                PEPROCESS EProcess = NULL;
                HANDLE UserEvent;
                PKEVENT kEvent;
                ULONG NeedLen = 0;
                ULONG ReturnLength = 0;

                if (cbInputBuffer == sizeof(HANDLE) * 2) {
                    UserEvent = *(PHANDLE)InputBuffer;
                    ntStatus = ObReferenceObjectByHandle(UserEvent, 0, 
                                *ExEventObjectType, UserMode, &kEvent, NULL);
                    if (NT_SUCCESS(ntStatus)) {
                        ScObQueryDriverObject(pdoGlobalDrvObj, kEvent);
                        ObDereferenceObject(kEvent);
                    }
                    Irp->IoStatus.Information = 0;  break;
                }

                DriverHead = ScObQueryDriverObject(NULL, NULL);
                if (DriverHead == NULL) {
                    ntStatus = STATUS_UNSUCCESSFUL;  break;
                }
                NeedLen = DriverHead->NumberOfDrivers * sizeof(DRIVER_INFO);

                if (cbOutputBuffer < NeedLen) {
                    if (cbOutputBuffer == sizeof(ULONG)) {
                        ((PULONG)OutputBuffer)[0] = NeedLen;
                        Irp->IoStatus.Information = sizeof(ULONG);
                        break;
                    }
                    ntStatus = STATUS_BUFFER_TOO_SMALL;  break;
                }
                ReturnLength = ExCopyDriverList2Buffer((PDRIVER_INFO)OutputBuffer);
                if (ReturnLength == 0)  ntStatus = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = ReturnLength;
                break;
            }
        //////////////////////////////////////////////////////////////////////////
        case IOCTL_LIST_DIRECTORY:
            {
                PWCHAR pszDirectory;
                ULONG NeedLength;
                ULONG ReturnLength;
                PFILE_LIST_HEAD FileHead;

                pszDirectory = ExAllocatePoolWithTag(PagedPool, 260 * 2, MEM_TAG);
                RtlZeroMemory(pszDirectory, 260 * 2);

                if (cbInputBuffer == 260 * 2) {
                    RtlCopyMemory(pszDirectory, InputBuffer, 260 * 2);
                } else {
                    ntStatus = STATUS_BUFFER_TOO_SMALL;  break;
                }

                FileHead = ScfsQueryDirectoryInformation(pszDirectory);
                if (FileHead == NULL) {
                    ((PULONG)OutputBuffer)[0] = 0;
                    Irp->IoStatus.Information = sizeof(ULONG);
                    ntStatus = STATUS_SUCCESS;  break;
                }
                NeedLength = FileHead->NumberOfItems * sizeof(FILE_INFO);

                if (cbOutputBuffer < NeedLength) {
                    if (cbOutputBuffer == sizeof(ULONG)) {
                        ((PULONG)OutputBuffer)[0] = NeedLength;
                        Irp->IoStatus.Information = sizeof(ULONG);
                        break;
                    }
                    ntStatus = STATUS_BUFFER_TOO_SMALL; break;
                }
                ReturnLength = ExCopyFileList2Buffer((PFILE_INFO)OutputBuffer);
                if (ReturnLength == 0)  ntStatus = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = ReturnLength;
                ExFreePoolWithTag(pszDirectory, MEM_TAG);
                break;
            }
        //////////////////////////////////////////////////////////////////////////
        case IOCTL_PROTECT_MYSELF:
            {
                HANDLE ProcessId;
                if (cbInputBuffer == sizeof(ULONG)) {
                    ProcessId = ((PHANDLE)InputBuffer)[0];
                    if (ProcessId) {
                        ntStatus = ScPtHideProcessById(ProcessId);
                    }
                }
                Irp->IoStatus.Information = 0;
                break;
            }
        //////////////////////////////////////////////////////////////////////////
        case IOCTL_EXIT_PROCESS:
            ScPtUnloadRoutine();
            break;
        //////////////////////////////////////////////////////////////////////////
        default:
            Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;
            break;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ntStatus = GetExceptionCode();
        Irp->IoStatus.Information = 0;
    }
  
    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return ntStatus;
}

VOID ScDetective_DriverUnload(
    IN PDRIVER_OBJECT		DriverObject
    )
{
    PDEVICE_OBJECT pdoNextDeviceObj = pdoGlobalDrvObj->DeviceObject;

    KdPrint(("[ScDetective_DriverUnload] Unloading..."));

    IoDeleteSymbolicLink(&usSymlinkName);

    // Delete all the device objects
    while(pdoNextDeviceObj)
    {
        PDEVICE_OBJECT pdoThisDeviceObj = pdoNextDeviceObj;
        pdoNextDeviceObj = pdoThisDeviceObj->NextDevice;
        IoDeleteDevice(pdoThisDeviceObj);
    }
    KdPrint(("[ScDetective_DriverUnload] Unload finished"));
}

#ifdef __cplusplus
extern "C" {
#endif
NTSTATUS DriverEntry(
    IN OUT PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING      RegistryPath
    )
{
    PDEVICE_OBJECT pdoDeviceObj = 0;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    pdoGlobalDrvObj = DriverObject;
   
    status = InitializeScDetective();
    if (!NT_SUCCESS(status))  return status;
    
    // Create the device object.
    if(!NT_SUCCESS(status = IoCreateDevice(
        DriverObject,
        0,
        &usDeviceName,
        FILE_DEVICE_SEDECTIVE,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &pdoDeviceObj
        )))
    {
        // Bail out (implicitly forces the driver to unload).
        return status;
    };

    // Now create the respective symbolic link object
    if(!NT_SUCCESS(status = IoCreateSymbolicLink(
        &usSymlinkName,
        &usDeviceName
        ))) 
    {
        IoDeleteDevice(pdoDeviceObj);
        return status;
    }
    
    // NOTE: You need not provide your own implementation for any major function that
    //       you do not want to handle. I have seen code using DDKWizard that left the
    //       *empty* dispatch routines intact. This is not necessary at all!
    DriverObject->MajorFunction[IRP_MJ_CREATE] = ScDetective_DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ScDetective_DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ScDetective_DispatchDeviceControl;
    DriverObject->DriverUnload = ScDetective_DriverUnload;
    
    return STATUS_SUCCESS;
}
#ifdef __cplusplus
}; // extern "C"
#endif
