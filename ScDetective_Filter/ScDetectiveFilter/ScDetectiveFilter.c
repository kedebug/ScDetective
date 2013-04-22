///////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2010 - <company name here>
///
/// Original filename: ScDetectiveFilter.cpp
/// Project          : ScDetectiveFilter
/// Date of creation : 2010-11-29
/// Author(s)        : <author name(s)>
///
/// Purpose          : <description>
///
/// Revisions:
///  0000 [2010-11-29] Initial revision.
///
///////////////////////////////////////////////////////////////////////////////

// $Id$


#include <ntddk.h>
#include <string.h>
#include "ScDetectiveFilter.h"

PDRIVER_OBJECT pdoGlobalDrvObj = NULL;
PDEVICE_OBJECT pdoGlobalDeviceObject = NULL;
LIST_ENTRY pdoGlobalHideObHead;

BOOLEAN
IS_MY_DEVICE_OBJECT(PDEVICE_OBJECT DeviceObject)
{
    return DeviceObject->DriverObject == pdoGlobalDrvObj;
}

BOOLEAN
IS_MY_HIDE_OBJECT(const PWCHAR Name, ULONG NameLength, ULONG Flag)
{
    PLIST_ENTRY ListHead = &pdoGlobalHideObHead;
    PLIST_ENTRY ListEntry = ListHead;
    PHIDE_OBJECT HideObject;
    ULONG ObFlag = (Flag & FILE_ATTRIBUTE_DIRECTORY) ? CDO_FLAG_DIRECTORY : CDO_FLAG_FILE;

    if (IsListEmpty(ListHead))  return FALSE;

    while (ListEntry->Flink != ListHead)
    {
        ListEntry = ListEntry->Flink;
        HideObject = (PHIDE_OBJECT)CONTAINING_RECORD(ListEntry, HIDE_OBJECT, ObjectLink);

        if (_wcsnicmp(Name, HideObject->ObjectName, NameLength >> 2) == 0 &&
            ObFlag == HideObject->HideFlag) {
            KdPrint(("[IS_MY_HIDE_OBJECT] Found hide object : %ws", Name));  return TRUE;
        }
    }
    return FALSE;
}

VOID AddObject2Hide(PWCHAR Name, ULONG Flag)
{
    PHIDE_OBJECT HideObject;

    HideObject = ExAllocatePoolWithTag(PagedPool, sizeof(HIDE_OBJECT), MEM_TAG);
    HideObject->HideFlag = Flag;
    wcscpy(HideObject->ObjectName, Name);
    InsertHeadList(&pdoGlobalHideObHead, &HideObject->ObjectLink);
    KdPrint(("New object to hide : %ws", Name));
}

VOID HideMyself(IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS ntStatus;
    HANDLE hKey;
    OBJECT_ATTRIBUTES ObAttributes;
    UNICODE_STRING ValueName;
    ULONG ReturnLength = 0;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    PWCHAR NamePos;

    InitializeObjectAttributes(&ObAttributes, RegistryPath, 0, NULL, NULL);

    ntStatus = ZwOpenKey(&hKey, KEY_READ, &ObAttributes);

    if (!NT_SUCCESS(ntStatus))  return ;

    RtlInitUnicodeString(&ValueName, L"ImagePath");

    ntStatus = ZwQueryValueKey(hKey, &ValueName, KeyValuePartialInformation, 
                               NULL, 0, &ReturnLength);

    if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND || ReturnLength == 0)  return;
 
    KeyValueInformation = ExAllocatePoolWithTag(NonPagedPool, ReturnLength, MEM_TAG);

    ntStatus = ZwQueryValueKey(hKey, &ValueName, KeyValuePartialInformation,
                               KeyValueInformation, ReturnLength, &ReturnLength);

    if (NT_SUCCESS(ntStatus)) {

        NamePos = (PWCHAR)(KeyValueInformation->Data + KeyValueInformation->DataLength);
        do {
            NamePos --;
        } while (NamePos[0] != L'\\');

        AddObject2Hide(NamePos + 1, CDO_FLAG_FILE);
    }

    ZwClose(hKey);
    ExFreePoolWithTag(KeyValueInformation, MEM_TAG);
    return ;
}

VOID
ScfsGetObjectName(
	IN PVOID Object,
	IN OUT PUNICODE_STRING Name
    )
{
    NTSTATUS ntStatus;
    CCHAR Buffer[512];
    ULONG ReturnLength;
    POBJECT_NAME_INFORMATION NameInfo = (POBJECT_NAME_INFORMATION)Buffer;
    
    ntStatus = ObQueryNameString(Object, NameInfo, sizeof(Buffer), &ReturnLength);

    Name->Length = 0;

    if (NT_SUCCESS(ntStatus)) {
        RtlCopyUnicodeString(Name, &(NameInfo->Name));
    }
}

NTSTATUS 
AttachToDiskDevice(IN PUNICODE_STRING DiskName, OUT PDEVICE_OBJECT* ReturnedDevice)
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION DevExt;
    PDEVICE_OBJECT DiskDevice = NULL;
    PDEVICE_OBJECT FilterDevice = NULL;
    IO_STATUS_BLOCK ioStatus;
    OBJECT_ATTRIBUTES obAttributes;
    PFILE_OBJECT FileObject = NULL;
    HANDLE FileHandle;

    InitializeObjectAttributes(&obAttributes, DiskName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    ntStatus = ZwCreateFile(&FileHandle, SYNCHRONIZE | FILE_ANY_ACCESS,
                            &obAttributes, &ioStatus, NULL, 0,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN,
                            FILE_SYNCHRONOUS_IO_ALERT | FILE_DIRECTORY_FILE, NULL, 0);

    if (!NT_SUCCESS(ntStatus))  return ntStatus;

    ntStatus = ObReferenceObjectByHandle(FileHandle, FILE_READ_DATA, NULL, 
                                         KernelMode, &FileObject, NULL);

    if (!NT_SUCCESS(ntStatus)) {
        ZwClose(FileHandle);
        return ntStatus;
    }

    DiskDevice = IoGetRelatedDeviceObject(FileObject);

    if (DiskDevice == NULL)  goto _ErrorHandle;

    ntStatus = IoCreateDevice(pdoGlobalDrvObj, 
                              sizeof(DEVICE_EXTENSION), 
                              NULL, 
                              DiskDevice->DeviceType, 
                              FILE_DEVICE_SECURE_OPEN,
                              FALSE, 
                              &FilterDevice);

    if (!NT_SUCCESS(ntStatus))  goto _ErrorHandle;

    FilterDevice->Flags &= ~ DO_DEVICE_INITIALIZING;
    ReturnedDevice[0] = FilterDevice;

    DevExt = FilterDevice->DeviceExtension;
    DevExt->DeviceName.Length = 0;
    DevExt->DeviceName.MaximumLength = sizeof(DevExt->NameBuffer);
    DevExt->DeviceName.Buffer = DevExt->NameBuffer;

    ScfsGetObjectName(pdoGlobalDeviceObject, &DevExt->DeviceName);

    DevExt->AttachedDevice = IoAttachDeviceToDeviceStack(FilterDevice, DiskDevice);

    if (DevExt->AttachedDevice == NULL)  goto _ErrorHandle;

    KdPrint(("[AttachToDiskDevice] attach succeed..."));
    ObDereferenceObject(FileObject);
    ZwClose(FileHandle);
    return STATUS_SUCCESS;

_ErrorHandle:
    KdPrint(("[AttachToDiskDevice] Operation failed : 0x%08x", ntStatus));
    ObDereferenceObject(FileObject);
    ZwClose(FileHandle);
    return ntStatus;
}

NTSTATUS ScfsPassThrough (
    IN PDEVICE_OBJECT		DeviceObject,
    IN PIRP					Irp
    )
{
    PDEVICE_EXTENSION DevExt = DeviceObject->DeviceExtension;

    if (pdoGlobalDeviceObject == DeviceObject) {
        __asm int 3;
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DevExt->AttachedDevice, Irp);
}



NTSTATUS ScfsDispatchCreate(
    IN PDEVICE_OBJECT		DeviceObject,
    IN PIRP					Irp
    )
{
    if (pdoGlobalDeviceObject == DeviceObject) {
        KdPrint(("cdo created"));
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }
    return ScfsPassThrough(DeviceObject, Irp);
}

NTSTATUS ScfsDispatchClose(
    IN PDEVICE_OBJECT		DeviceObject,
    IN PIRP					Irp
    )
{
    if (pdoGlobalDeviceObject == DeviceObject) {
        KdPrint(("cdo closed"));
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }
    return ScfsPassThrough(DeviceObject, Irp);
}

BOOLEAN
HandleDirectory(
    IN OUT PFILE_BOTH_DIR_INFORMATION DirInfo, 
    IN PULONG BufferSize
    )
{
    PFILE_BOTH_DIR_INFORMATION NowDirInfo = DirInfo;
    PFILE_BOTH_DIR_INFORMATION LastDirInfo = NULL;
    ULONG offset = 0;
    ULONG position = 0;
    ULONG NewSize = BufferSize[0];

    do {
        offset = NowDirInfo->NextEntryOffset;

        if (IS_MY_HIDE_OBJECT(NowDirInfo->FileName, 
                      NowDirInfo->FileNameLength, 
                      NowDirInfo->FileAttributes)) {

            KdPrint(("%08x Hided File:%ws[%d]\n", 
                NowDirInfo->FileAttributes, NowDirInfo->FileName, NowDirInfo->FileNameLength));

            if (offset == 0) {
                if (LastDirInfo) {
                    LastDirInfo->NextEntryOffset = 0;
                    NewSize -= BufferSize[0] - position;
                } else {
                    NowDirInfo->NextEntryOffset = 0;
                    BufferSize[0] = 0;
                    return TRUE;
                }
            } else {
                RtlMoveMemory(NowDirInfo, (PUCHAR)NowDirInfo + offset, BufferSize[0] - position - offset);
                NewSize -= offset;
                position += offset;
            }
        } else {
            position += offset;
            LastDirInfo = NowDirInfo;
            NowDirInfo = (PFILE_BOTH_DIR_INFORMATION)((ULONG)NowDirInfo + offset);
        }
    } while (offset != 0);
    
    BufferSize[0] = NewSize;
    return TRUE;
}

NTSTATUS
DirControlCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    //if (Irp->PendingReturned) IoMarkIrpPending(Irp);
    KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;	//注：必须返回这个值
}

NTSTATUS ScfsDirectoryControl(
    IN PDEVICE_OBJECT		DeviceObject,
    IN PIRP					Irp
    )
{
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION DevExt = DeviceObject->DeviceExtension;
    KEVENT kEvent;
    PQUERY_DIRECTORY QueryDir;

    ASSERT(pdoGlobalDeviceObject != DeviceObject);
    ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));

    if (irpSp->MinorFunction != IRP_MN_QUERY_DIRECTORY)  goto _SkipHandle;
    if (Irp->RequestorMode == KernelMode)  goto _SkipHandle;

    QueryDir = (PQUERY_DIRECTORY)&(irpSp->Parameters);
    if (QueryDir->FileInformationClass != FileBothDirectoryInformation)  goto _SkipHandle;

    KeInitializeEvent(&kEvent, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, DirControlCompletion, &kEvent, TRUE, TRUE, TRUE);

    ntStatus = IoCallDriver(DevExt->AttachedDevice, Irp);

    if (ntStatus == STATUS_PENDING) {
        ntStatus = KeWaitForSingleObject(&kEvent, Executive, KernelMode, FALSE, NULL);
        ASSERT(ntStatus == STATUS_SUCCESS);
    }

    if (!NT_SUCCESS(ntStatus) || irpSp->Parameters.QueryFile.Length == 0) {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);  
        return ntStatus;
    }
    
    QueryDir = (PQUERY_DIRECTORY)&(irpSp->Parameters);
    HandleDirectory(Irp->UserBuffer, &QueryDir->Length);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return ntStatus;
  
_SkipHandle:
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DevExt->AttachedDevice, Irp);
}

NTSTATUS ScfsDispatchDeviceControl(
    IN PDEVICE_OBJECT		DeviceObject,
    IN PIRP					Irp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    if (pdoGlobalDeviceObject != DeviceObject) {
        return ScfsPassThrough(DeviceObject, Irp);
    }
/*
    switch(irpSp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_SCDETECTIVEFILTER_OPERATION:
        // status = SomeHandlerFunction(irpSp);
        break;
    default:
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        break;
    }
*/
    status = Irp->IoStatus.Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

VOID ScfsDriverUnload(
    IN PDRIVER_OBJECT		DriverObject
    )
{
    PDEVICE_OBJECT pdoNextDeviceObj = pdoGlobalDrvObj->DeviceObject;
    PDEVICE_OBJECT pdoThisDeviceObj;
    PLIST_ENTRY ListEntry;
    PHIDE_OBJECT HideObject;
    PDEVICE_EXTENSION DevExt = NULL;

    IoDeleteSymbolicLink(&usSymlinkName);

    while (!IsListEmpty(&pdoGlobalHideObHead))
    {
        ListEntry = RemoveHeadList(&pdoGlobalHideObHead);
        HideObject = (PHIDE_OBJECT)CONTAINING_RECORD(ListEntry, HIDE_OBJECT, ObjectLink);
        ExFreePoolWithTag(HideObject, MEM_TAG);
    }

    // Delete all the device objects
    while(pdoNextDeviceObj)
    {
        pdoThisDeviceObj = pdoNextDeviceObj;
        DevExt = pdoThisDeviceObj->DeviceExtension;

        if (DevExt) {
            if (DevExt->AttachedDevice) {
                IoDetachDevice(DevExt->AttachedDevice);
            }
        }
        pdoNextDeviceObj = pdoThisDeviceObj->NextDevice;
        IoDeleteDevice(pdoThisDeviceObj);
    }
    DestoryFastIo(DriverObject);
}

NTSTATUS DriverEntry(
    IN OUT PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING      RegistryPath
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    UNICODE_STRING NameString;
    PDEVICE_OBJECT FilterDevice;
    ULONG i;

    pdoGlobalDrvObj = DriverObject;

    // Create the device object.
    if(!NT_SUCCESS(status = IoCreateDevice(
        DriverObject,
        0,
        &usDeviceName,
        FILE_DEVICE_DISK_FILE_SYSTEM,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &pdoGlobalDeviceObject
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
        IoDeleteDevice(pdoGlobalDeviceObject);
        return status;
    }

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        DriverObject->MajorFunction[i] = ScfsPassThrough;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = ScfsDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ScfsDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] = ScfsDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ScfsDispatchDeviceControl;
    DriverObject->DriverUnload = ScfsDriverUnload;

    InitFastIo(DriverObject);

    // 绑定 C:\ 设备
    RtlInitUnicodeString(&NameString, L"\\DosDevices\\C:\\");

    if (!NT_SUCCESS(status = AttachToDiskDevice(
        &NameString, 
        &FilterDevice
        ))) 
    {
        IoDeleteDevice(FilterDevice);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    
    InitializeListHead(&pdoGlobalHideObHead);
    HideMyself(RegistryPath);
    AddObject2Hide(L"12.txt", CDO_FLAG_FILE);
    AddObject2Hide(L"boot.ini", CDO_FLAG_FILE);
    return STATUS_SUCCESS;
}

