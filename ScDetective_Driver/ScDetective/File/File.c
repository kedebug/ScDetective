
#include "File.h"

//////////////////////////////////////////////////////////////////////////

NTSTATUS
ScfsQueryFileNameString(PFILE_OBJECT FileObject, PUNICODE_STRING NameString)
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DosDeviceName;
    USHORT BufferSize;

    if (!MmIsAddressValid(FileObject))  return STATUS_ACCESS_DENIED;
    if (!MmIsAddressValid(NameString->Buffer))  return STATUS_ACCESS_DENIED;

    ASSERT(FileObject != NULL && NameString != NULL);

    __try {
        DeviceObject = FileObject->DeviceObject;
        ntStatus = IoVolumeDeviceToDosName(DeviceObject, &DosDeviceName);

        if (!NT_SUCCESS(ntStatus))  return ntStatus;

        BufferSize = FileObject->FileName.Length + DosDeviceName.Length + sizeof(WCHAR);

        if (NameString->MaximumLength < BufferSize) {
            ExFreePool(DosDeviceName.Buffer);
            return STATUS_BUFFER_TOO_SMALL;
        }

        RtlZeroMemory(NameString->Buffer, NameString->MaximumLength);
        RtlCopyMemory(NameString->Buffer, DosDeviceName.Buffer, DosDeviceName.Length);
        RtlCopyMemory((PVOID)((ULONG)NameString->Buffer + DosDeviceName.Length), 
                      FileObject->FileName.Buffer, FileObject->FileName.Length);

        NameString->Length = BufferSize;
        ExFreePool(DosDeviceName.Buffer);
    } 
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
    return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

PUNICODE_STRING Convert2KernelLinkName(PUNICODE_STRING DosLinkName)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    OBJECT_ATTRIBUTES ObAttributes;
    HANDLE LinkHandle;
    PUNICODE_STRING KernelLinkName = NULL;
    ULONG ReturnLength = 0;

    InitializeObjectAttributes(&ObAttributes, DosLinkName, OBJ_CASE_INSENSITIVE, 0, 0);

    ntStatus = ZwOpenSymbolicLinkObject(&LinkHandle, MAXIMUM_ALLOWED, &ObAttributes);

    if (!NT_SUCCESS(ntStatus))  return NULL;

    KernelLinkName = ExAllocatePoolWithTag(NonPagedPool, 0x208, MEM_TAG);
    KernelLinkName->Buffer = (PWCH)((ULONG)KernelLinkName + 8);
    KernelLinkName->Length = 0;
    KernelLinkName->MaximumLength = 0x200;

    ntStatus = ZwQuerySymbolicLinkObject(LinkHandle, KernelLinkName, &ReturnLength);

    if (NT_SUCCESS(ntStatus)) {
        ZwClose(LinkHandle);
        return KernelLinkName;
    }

    ZwClose(LinkHandle);

    if (KernelLinkName) {
        ExFreePoolWithTag(KernelLinkName, MEM_TAG);
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////

NTSTATUS QueryCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PIO_STATUS_BLOCK ioStatus;

    ioStatus = Irp->UserIosb;
    ioStatus->Status = Irp->IoStatus.Status;
    ioStatus->Information = Irp->IoStatus.Information;

    KeSetEvent(Irp->UserEvent, 0, FALSE);
    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

//////////////////////////////////////////////////////////////////////////

PFILE_LIST_HEAD
ScfsQueryDirectoryInformation(PWCHAR pszDirectory)
{
    NTSTATUS ntStatus;
    WCHAR Buffer[260] = {0};
    OBJECT_ATTRIBUTES ObAttributes;
    UNICODE_STRING FileName;
    HANDLE FileHandle;
    IO_STATUS_BLOCK ioStatus;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT FileDevice;
    PIRP NewIrp = NULL;
    PIO_STACK_LOCATION IrpSp;
    KEVENT waitEvent;
    PVOID FileUserBuffer;
    PFILE_DIRECTORY_INFORMATION NowFileDirectory;
    PFILE_INFO NewFile;
    TIME_FIELDS testNumber;

    if (g_FileListHead != NULL)  return g_FileListHead;

    g_FileListHead = ExAllocatePoolWithTag(NonPagedPool, sizeof(FILE_LIST_HEAD), MEM_TAG);
    InitializeListHead(&g_FileListHead->FileListHead);
    g_FileListHead->NumberOfItems = 0;

#pragma warning(disable:4995)
    wcscpy(Buffer, L"\\DosDevices\\");
    wcscat(Buffer, pszDirectory);

    RtlInitUnicodeString(&FileName, Buffer);

    InitializeObjectAttributes(&ObAttributes, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    ntStatus = ZwOpenFile(&FileHandle, 
                          FILE_LIST_DIRECTORY | FILE_ANY_ACCESS,
                          &ObAttributes, 
                          &ioStatus,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_ALERT);

    if (!NT_SUCCESS(ntStatus)) {
        ExFreePoolWithTag(g_FileListHead, MEM_TAG);
        g_FileListHead = NULL;
        return g_FileListHead;
    }

    ntStatus = ObReferenceObjectByHandle(FileHandle, 
                                         FILE_LIST_DIRECTORY | SYNCHRONIZE,
                                         NULL, 
                                         KernelMode,
                                         &FileObject,
                                         NULL);

    if (!NT_SUCCESS(ntStatus)) {
        ZwClose(FileHandle);
        ExFreePoolWithTag(g_FileListHead, MEM_TAG);
        g_FileListHead = NULL;
        return g_FileListHead;
    }

    FileDevice = IoGetRelatedDeviceObject(FileObject);

    NewIrp = IoAllocateIrp(FileDevice->StackSize, FALSE);

    if (NewIrp == NULL) {
        ObDereferenceObject(FileObject);
        ZwClose(FileHandle);
        ExFreePoolWithTag(g_FileListHead, MEM_TAG);
        g_FileListHead = NULL;
        return g_FileListHead;
    }

    KeInitializeEvent(&waitEvent, NotificationEvent, FALSE);
    FileUserBuffer = ExAllocatePoolWithTag(NonPagedPool, 655350, MEM_TAG);

    NewIrp->UserEvent = &waitEvent;
    NewIrp->UserBuffer = FileUserBuffer;
    NewIrp->AssociatedIrp.SystemBuffer = FileUserBuffer;
    NewIrp->MdlAddress  = NULL;
    NewIrp->Flags = 0;
    NewIrp->UserIosb = &ioStatus;
    NewIrp->Tail.Overlay.OriginalFileObject = FileObject;
    NewIrp->Tail.Overlay.Thread = PsGetCurrentThread();
    NewIrp->RequestorMode = KernelMode;
    
    IrpSp = IoGetNextIrpStackLocation(NewIrp);
    IrpSp->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    IrpSp->MinorFunction = IRP_MN_QUERY_DIRECTORY;
    IrpSp->FileObject = FileObject;
    IrpSp->DeviceObject = FileDevice;
    IrpSp->Flags = SL_RESTART_SCAN;
    IrpSp->Control = 0;
    IrpSp->Parameters.QueryDirectory.FileIndex = 0;
    IrpSp->Parameters.QueryDirectory.FileInformationClass = FileDirectoryInformation;
    IrpSp->Parameters.QueryDirectory.FileName = NULL;
    IrpSp->Parameters.QueryDirectory.Length = 655350;
  
    IoSetCompletionRoutine(NewIrp, QueryCompletion, NULL, TRUE, TRUE, TRUE);
    IoCallDriver(FileDevice, NewIrp);
    
    KeWaitForSingleObject(&waitEvent, Executive, KernelMode, TRUE, FALSE);

    NowFileDirectory = (PFILE_DIRECTORY_INFORMATION)FileUserBuffer;
    
    while (TRUE)
    {
        RtlTimeToTimeFields(&NowFileDirectory->CreationTime, &testNumber);
        if (testNumber.Month == 0 && testNumber.Day == 0)  goto _RetryNext;

        if ((NowFileDirectory->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            (NowFileDirectory->FileName)[0] == L'.')  goto _RetryNext;

        NewFile = ExAllocatePoolWithTag(PagedPool, sizeof(FILE_INFO), MEM_TAG);
        RtlZeroMemory(NewFile, sizeof(FILE_INFO));

        RtlCopyMemory(NewFile->FileName, NowFileDirectory->FileName, NowFileDirectory->FileNameLength);
        NewFile->AllocationSize = NowFileDirectory->AllocationSize;
        NewFile->EndOfFile = NowFileDirectory->EndOfFile;
        NewFile->FileAttributes = NowFileDirectory->FileAttributes;
        RtlTimeToTimeFields(&NowFileDirectory->CreationTime, &NewFile->CreationTime);
        RtlTimeToTimeFields(&NowFileDirectory->LastWriteTime, &NewFile->LastWriteTime);

        InsertHeadList(&g_FileListHead->FileListHead, &NewFile->FileLink);
        g_FileListHead->NumberOfItems ++;
_RetryNext:
        if (NowFileDirectory->NextEntryOffset == 0)  break;

        NowFileDirectory = (PFILE_DIRECTORY_INFORMATION)((ULONG)NowFileDirectory + NowFileDirectory->NextEntryOffset);
    }

    ExFreePoolWithTag(FileUserBuffer, MEM_TAG);
    ObDereferenceObject(FileObject);
    ZwClose(FileHandle);

    return g_FileListHead;
}

//////////////////////////////////////////////////////////////////////////

ULONG ExCopyFileList2Buffer(PFILE_INFO FileInfo)
{
    PFILE_INFO tempFile;
    ULONG ReturnLength = 0;

    if (g_FileListHead == NULL)  return 0;

    while (!IsListEmpty(&g_FileListHead->FileListHead))
    {
        tempFile = (PFILE_INFO)RemoveHeadList(&g_FileListHead->FileListHead);
        RtlCopyMemory(FileInfo, tempFile, sizeof(FILE_INFO));
        ExFreePoolWithTag(tempFile, MEM_TAG);
        FileInfo ++;
        ReturnLength ++;
    }

    ExFreePoolWithTag(g_FileListHead, MEM_TAG);
    g_FileListHead = NULL;
    return ReturnLength * sizeof(FILE_INFO);
}
/*
//////////////////////////////////////////////////////////////////////////

#include <ntddk.h>

#define NT_DEVICE_NAME                L"\\Device\\SuperKill"
#define DOS_DEVICE_NAME               L"\\DosDevices\\SuperKill"


VOID 
SKillUnloadDriver( 
                  IN PDRIVER_OBJECT    DriverObject 
                  )
{
    PDEVICE_OBJECT    deviceObject = DriverObject->DeviceObject;
    UNICODE_STRING    uniSymLink;

    RtlInitUnicodeString(&uniSymLink, DOS_DEVICE_NAME);

    IoDeleteSymbolicLink(&uniSymLink);

    IoDeleteDevice(deviceObject);
}


HANDLE
SkillIoOpenFile(
                IN PCWSTR FileName,
                IN ACCESS_MASK DesiredAccess,
                IN ULONG ShareAccess
                )
{
    NTSTATUS              ntStatus;
    UNICODE_STRING        uniFileName;
    OBJECT_ATTRIBUTES     objectAttributes;
    HANDLE                ntFileHandle;
    IO_STATUS_BLOCK       ioStatus;

    if (KeGetCurrentIrql() > PASSIVE_LEVEL)
    {
        return 0;
    }

    RtlInitUnicodeString(&uniFileName, FileName);

    InitializeObjectAttributes(&objectAttributes, &uniFileName,
        OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    ntStatus = IoCreateFile(&ntFileHandle,
        DesiredAccess,
        &objectAttributes,
        &ioStatus,
        0,
        FILE_ATTRIBUTE_NORMAL,
        ShareAccess,
        FILE_OPEN,
        0,
        NULL,
        0,
        0,
        NULL,
        IO_NO_PARAMETER_CHECKING);

    if (!NT_SUCCESS(ntStatus))
    {
        return 0;
    }

    return ntFileHandle;
}

NTSTATUS
SkillSetFileCompletion(
                       IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp,
                       IN PVOID Context
                       )
{
    Irp->UserIosb->Status = Irp->IoStatus.Status;
    Irp->UserIosb->Information = Irp->IoStatus.Information;

    KeSetEvent(Irp->UserEvent, IO_NO_INCREMENT, FALSE);

    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

BOOLEAN
SKillStripFileAttributes(
                         IN HANDLE    FileHandle
                         )
{
    NTSTATUS          ntStatus = STATUS_SUCCESS;
    PFILE_OBJECT      fileObject;
    PDEVICE_OBJECT    DeviceObject;
    PIRP              Irp;
    KEVENT            event1;
    FILE_BASIC_INFORMATION    FileInformation;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION irpSp;

    ntStatus = ObReferenceObjectByHandle(FileHandle,
        DELETE,
        *IoFileObjectType,
        KernelMode,
        &fileObject,
        NULL);//我想知道的是这个文件句柄是在哪个进程的句柄表中

    if (!NT_SUCCESS(ntStatus))
    {
        return FALSE;
    }

    DeviceObject = IoGetRelatedDeviceObject(fileObject);
    Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);

    if (Irp == NULL)
    {
        ObDereferenceObject(fileObject);
        return FALSE;
    }

    KeInitializeEvent(&event1, SynchronizationEvent, FALSE);

    memset(&FileInformation,0,0x28);

    FileInformation.FileAttributes = FILE_ATTRIBUTE_NORMAL;
    Irp->AssociatedIrp.SystemBuffer = &FileInformation;
    Irp->UserEvent = &event1;
    Irp->UserIosb = &ioStatus;
    Irp->Tail.Overlay.OriginalFileObject = fileObject;
    Irp->Tail.Overlay.Thread = (PETHREAD)KeGetCurrentThread();
    Irp->RequestorMode = KernelMode;

    irpSp = IoGetNextIrpStackLocation(Irp);
    irpSp->MajorFunction = IRP_MJ_SET_INFORMATION;
    irpSp->DeviceObject = DeviceObject;
    irpSp->FileObject = fileObject;
    irpSp->Parameters.SetFile.Length = sizeof(FILE_BASIC_INFORMATION);
    irpSp->Parameters.SetFile.FileInformationClass = FileBasicInformation;
    irpSp->Parameters.SetFile.FileObject = fileObject;

    IoSetCompletionRoutine(
        Irp,
        SkillSetFileCompletion,
        &event1,
        TRUE,
        TRUE,
        TRUE);

    IoCallDriver(DeviceObject, Irp);//调用这个设备对象的驱动对象，并且ＩＯ＿ＳｔＡＣＫ＿ＬＯＣＡｔｉｏｎ会指向下一个，也就是刚刚设置的
    　　　　　　　　　　　　　　　　//如果没有文件系统驱动建立的设备对象没有Attacked的话，就调用文件系统驱动的IRP_MJ_SET_INFORMATION分派例程


    //会调用NTFS.sys驱动的NtfsFsdSetInformation例程，再会进入NtfsSetBasicInfo（）函数，最后它会设置代表此文件的FCB（文件
    //控制块结构的一些信息，用来设置代表此文件的属性。最后不知道在哪里会调用IoCompleteRequest,它会依次调用先前设置的回调函数
    //回调函数会释放刚分配的IRP和设置事件对象为受信状态。
    KeWaitForSingleObject(&event1, Executive, KernelMode, TRUE, NULL);//一等到事件对象变成受信状态就会继续向下执行。

    ObDereferenceObject(fileObject);

    return TRUE;
}


BOOLEAN
SKillDeleteFile(
                IN HANDLE    FileHandle
                )
{
    NTSTATUS          ntStatus = STATUS_SUCCESS;
    PFILE_OBJECT      fileObject;
    PDEVICE_OBJECT    DeviceObject;
    PIRP              Irp;
    KEVENT            event1;
    FILE_DISPOSITION_INFORMATION    FileInformation;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION irpSp;
    PSECTION_OBJECT_POINTERS pSectionObjectPointer;     ////////////////////

    SKillStripFileAttributes( FileHandle);          //去掉只读属性，才能删除只读文件

    ntStatus = ObReferenceObjectByHandle(FileHandle,
        DELETE,
        *IoFileObjectType,
        KernelMode,
        &fileObject,
        NULL);

    if (!NT_SUCCESS(ntStatus))
    {
        return FALSE;
    }

    DeviceObject = IoGetRelatedDeviceObject(fileObject);//如果NTFS.sys驱动建立的设备对象上没有附加的设备对象的话，就返回NTFS.sys建立的设备对象
    //否则返回的是这个设备对象的highest level设备对象。
    Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);//如果没有附加，StackSize为7

    if (Irp == NULL)
    {
        ObDereferenceObject(fileObject);
        return FALSE;
    }

    KeInitializeEvent(&event1, SynchronizationEvent, FALSE);

    FileInformation.DeleteFile = TRUE;

    Irp->AssociatedIrp.SystemBuffer = &FileInformation;
    Irp->UserEvent = &event1;
    Irp->UserIosb = &ioStatus;
    Irp->Tail.Overlay.OriginalFileObject = fileObject;
    Irp->Tail.Overlay.Thread = (PETHREAD)KeGetCurrentThread();
    Irp->RequestorMode = KernelMode;

    irpSp = IoGetNextIrpStackLocation(Irp);            //得到文件系统NTFS.sys驱动的设备IO_STACK_LOCATION
    irpSp->MajorFunction = IRP_MJ_SET_INFORMATION;
    irpSp->DeviceObject = DeviceObject;
    irpSp->FileObject = fileObject;
    irpSp->Parameters.SetFile.Length = sizeof(FILE_DISPOSITION_INFORMATION);
    irpSp->Parameters.SetFile.FileInformationClass = FileDispositionInformation;
    irpSp->Parameters.SetFile.FileObject = fileObject;


    IoSetCompletionRoutine(
        Irp,
        SkillSetFileCompletion,
        &event1,
        TRUE,
        TRUE,
        TRUE);

    //再加上下面这三行代码 ，MmFlushImageSection    函数通过这个结构来检查是否可以删除文件。
    pSectionObjectPointer = fileObject->SectionObjectPointer;
    pSectionObjectPointer->ImageSectionObject = 0;
    pSectionObjectPointer->DataSectionObject = 0;


    IoCallDriver(DeviceObject, Irp);//这里会依次进入NTFS.sys驱动的NtfsFsdSetInformation例程->NtfsSetDispositionInfo（）->MmFlushImageSection(),
    //MmFlushImageSection（）这函数是用来检查FILE_OBJECT对象的SECTION_OBJECT_POINTER结构的变量，检查这个文件
    //在内存有没有被映射。也就是有没有执行。如果上面那样设置了，也就是说文件可以删除了。我们也可以HOOK NTFS.sys导入表中的
    //的MmFlushImageSection（），来检查这个文件对象是不是我们要删除 的，是的话，返回TRUE就行了。
    KeWaitForSingleObject(&event1, Executive, KernelMode, TRUE, NULL);

    ObDereferenceObject(fileObject);

    return TRUE;
}

NTSTATUS DriverEntry(
                     IN PDRIVER_OBJECT DriverObject,
                     IN PUNICODE_STRING RegistryPath
                     )
{
    UNICODE_STRING                  uniDeviceName;
    UNICODE_STRING                  uniSymLink;
    NTSTATUS                          ntStatus;
    PDEVICE_OBJECT                  deviceObject = NULL;
    HANDLE                hFileHandle;

    RtlInitUnicodeString(&uniDeviceName, NT_DEVICE_NAME);
    RtlInitUnicodeString(&uniSymLink, DOS_DEVICE_NAME);

    ntStatus = IoCreateDevice(
        DriverObject,
        0x100u,
        &uniDeviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        TRUE,
        &deviceObject);

    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    ntStatus = IoCreateSymbolicLink(&uniSymLink, &uniDeviceName);

    if (!NT_SUCCESS(ntStatus))
    {
        IoDeleteDevice(deviceObject);
        return ntStatus;
    }

    DriverObject->DriverUnload = SKillUnloadDriver;

    //
    // 重点在这
    //
    hFileHandle = SkillIoOpenFile(L"\\Device\\HarddiskVolume1\\test.exe", 
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_DELETE);   //得到文件句柄

    if (hFileHandle!=NULL)
    {
        SKillDeleteFile(hFileHandle);
        ZwClose(hFileHandle);
    }
    return STATUS_SUCCESS;
} 
*/