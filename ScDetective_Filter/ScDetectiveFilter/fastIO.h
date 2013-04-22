/*
typedef struct _FAST_IO_DISPATCH {
ULONG SizeOfFastIoDispatch;
PFAST_IO_CHECK_IF_POSSIBLE FastIoCheckIfPossible;
PFAST_IO_READ FastIoRead;
PFAST_IO_WRITE FastIoWrite;
PFAST_IO_QUERY_BASIC_INFO FastIoQueryBasicInfo;
PFAST_IO_QUERY_STANDARD_INFO FastIoQueryStandardInfo;
PFAST_IO_LOCK FastIoLock;
PFAST_IO_UNLOCK_SINGLE FastIoUnlockSingle;
PFAST_IO_UNLOCK_ALL FastIoUnlockAll;
PFAST_IO_UNLOCK_ALL_BY_KEY FastIoUnlockAllByKey;
PFAST_IO_DEVICE_CONTROL FastIoDeviceControl;
PFAST_IO_ACQUIRE_FILE AcquireFileForNtCreateSection;
PFAST_IO_RELEASE_FILE ReleaseFileForNtCreateSection;
PFAST_IO_DETACH_DEVICE FastIoDetachDevice;
PFAST_IO_QUERY_NETWORK_OPEN_INFO FastIoQueryNetworkOpenInfo;
PFAST_IO_ACQUIRE_FOR_MOD_WRITE AcquireForModWrite;
PFAST_IO_MDL_READ MdlRead;
PFAST_IO_MDL_READ_COMPLETE MdlReadComplete;
PFAST_IO_PREPARE_MDL_WRITE PrepareMdlWrite;
PFAST_IO_MDL_WRITE_COMPLETE MdlWriteComplete;
PFAST_IO_READ_COMPRESSED FastIoReadCompressed;
PFAST_IO_WRITE_COMPRESSED FastIoWriteCompressed;
PFAST_IO_MDL_READ_COMPLETE_COMPRESSED MdlReadCompleteCompressed;
PFAST_IO_MDL_WRITE_COMPLETE_COMPRESSED MdlWriteCompleteCompressed;
PFAST_IO_QUERY_OPEN FastIoQueryOpen;
PFAST_IO_RELEASE_FOR_MOD_WRITE ReleaseForModWrite;
PFAST_IO_ACQUIRE_FOR_CCFLUSH AcquireForCcFlush;
PFAST_IO_RELEASE_FOR_CCFLUSH ReleaseForCcFlush;
} FAST_IO_DISPATCH, *PFAST_IO_DISPATCH;
*/

#ifndef _FASTIO_H_
#define _FASTIO_H_
#include <ntddk.h>

#define SFLT_POOL_TAG  'SFLT'

BOOLEAN
SfFastIoCheckIfPossible (
                         IN PFILE_OBJECT FileObject,
                         IN PLARGE_INTEGER FileOffset,
                         IN ULONG Length,
                         IN BOOLEAN Wait,
                         IN ULONG LockKey,
                         IN BOOLEAN CheckForReadOperation,
                         OUT PIO_STATUS_BLOCK IoStatus,
                         IN PDEVICE_OBJECT DeviceObject)
{
    return FALSE;
}
BOOLEAN
SfFastIoRead(
             IN struct _FILE_OBJECT *FileObject,
             IN PLARGE_INTEGER FileOffset,
             IN ULONG Length,
             IN BOOLEAN Wait,
             IN ULONG LockKey,
             OUT PVOID Buffer,
             OUT PIO_STATUS_BLOCK IoStatus,
             IN struct _DEVICE_OBJECT *DeviceObject)
{
    return FALSE;
}

BOOLEAN
SfFastIoWrite (
               IN struct _FILE_OBJECT *FileObject,
               IN PLARGE_INTEGER FileOffset,
               IN ULONG Length,
               IN BOOLEAN Wait,
               IN ULONG LockKey,
               IN PVOID Buffer,
               OUT PIO_STATUS_BLOCK IoStatus,
               IN struct _DEVICE_OBJECT *DeviceObject
               )
{
    return FALSE;
}

BOOLEAN
SfFastIoQueryBasicInfo (
                        IN struct _FILE_OBJECT *FileObject,
                        IN BOOLEAN Wait,
                        OUT PFILE_BASIC_INFORMATION Buffer,
                        OUT PIO_STATUS_BLOCK IoStatus,
                        IN struct _DEVICE_OBJECT *DeviceObject
                        )
{
    return FALSE;
}

BOOLEAN
SfFastIoQueryStandardInfo (
                           IN struct _FILE_OBJECT *FileObject,
                           IN BOOLEAN Wait,
                           OUT PFILE_STANDARD_INFORMATION Buffer,
                           OUT PIO_STATUS_BLOCK IoStatus,
                           IN struct _DEVICE_OBJECT *DeviceObject
                           )
{
    return FALSE;
}

BOOLEAN
SfFastIoLock (
              IN struct _FILE_OBJECT *FileObject,
              IN PLARGE_INTEGER FileOffset,
              IN PLARGE_INTEGER Length,
              PEPROCESS ProcessId,
              ULONG Key,
              BOOLEAN FailImmediately,
              BOOLEAN ExclusiveLock,
              OUT PIO_STATUS_BLOCK IoStatus,
              IN struct _DEVICE_OBJECT *DeviceObject
              )
{
    return FALSE;
}

BOOLEAN
SfFastIoUnlockSingle (
                      IN struct _FILE_OBJECT *FileObject,
                      IN PLARGE_INTEGER FileOffset,
                      IN PLARGE_INTEGER Length,
                      PEPROCESS ProcessId,
                      ULONG Key,
                      OUT PIO_STATUS_BLOCK IoStatus,
                      IN struct _DEVICE_OBJECT *DeviceObject
                      )
{
    return FALSE;
}

BOOLEAN
SfFastIoUnlockAll (
                   IN struct _FILE_OBJECT *FileObject,
                   PEPROCESS ProcessId,
                   OUT PIO_STATUS_BLOCK IoStatus,
                   IN struct _DEVICE_OBJECT *DeviceObject
                   )
{
    return FALSE;
}

BOOLEAN
SfFastIoUnlockAllByKey (
                        IN struct _FILE_OBJECT *FileObject,
                        PVOID ProcessId,
                        ULONG Key,
                        OUT PIO_STATUS_BLOCK IoStatus,
                        IN struct _DEVICE_OBJECT *DeviceObject
                        )
{
    return FALSE;
}

BOOLEAN
SfFastIoDeviceControl (
                       IN struct _FILE_OBJECT *FileObject,
                       IN BOOLEAN Wait,
                       IN PVOID InputBuffer OPTIONAL,
                       IN ULONG InputBufferLength,
                       OUT PVOID OutputBuffer OPTIONAL,
                       IN ULONG OutputBufferLength,
                       IN ULONG IoControlCode,
                       OUT PIO_STATUS_BLOCK IoStatus,
                       IN struct _DEVICE_OBJECT *DeviceObject
                       )
{
    return FALSE;
}

VOID
SfFastIoAcquireFile (
                     IN struct _FILE_OBJECT *FileObject
                     )
{
}

VOID
SfFastIoReleaseFile (
                     IN struct _FILE_OBJECT *FileObject
                     )
{
}

VOID
SfFastIoDetachDevice (
                      IN struct _DEVICE_OBJECT *SourceDevice,
                      IN struct _DEVICE_OBJECT *TargetDevice
                      )
{
}

BOOLEAN
SfFastIoQueryNetworkOpenInfo (
                              IN struct _FILE_OBJECT *FileObject,
                              IN BOOLEAN Wait,
                              OUT struct _FILE_NETWORK_OPEN_INFORMATION *Buffer,
                              OUT struct _IO_STATUS_BLOCK *IoStatus,
                              IN struct _DEVICE_OBJECT *DeviceObject
                              )
{
    return FALSE;
}

BOOLEAN
SfFastIoMdlRead (
                 IN struct _FILE_OBJECT *FileObject,
                 IN PLARGE_INTEGER FileOffset,
                 IN ULONG Length,
                 IN ULONG LockKey,
                 OUT PMDL *MdlChain,
                 OUT PIO_STATUS_BLOCK IoStatus,
                 IN struct _DEVICE_OBJECT *DeviceObject
                 )
{
    return FALSE;
}

BOOLEAN
SfFastIoMdlReadComplete (
                         IN struct _FILE_OBJECT *FileObject,
                         IN PMDL MdlChain,
                         IN struct _DEVICE_OBJECT *DeviceObject
                         )
{
    return FALSE;
}

BOOLEAN
SfFastIoPrepareMdlWrite (
                         IN struct _FILE_OBJECT *FileObject,
                         IN PLARGE_INTEGER FileOffset,
                         IN ULONG Length,
                         IN ULONG LockKey,
                         OUT PMDL *MdlChain,
                         OUT PIO_STATUS_BLOCK IoStatus,
                         IN struct _DEVICE_OBJECT *DeviceObject
                         )
{
    return FALSE;
}

BOOLEAN
SfFastIoMdlWriteComplete (
                          IN struct _FILE_OBJECT *FileObject,
                          IN PLARGE_INTEGER FileOffset,
                          IN PMDL MdlChain,
                          IN struct _DEVICE_OBJECT *DeviceObject
                          )
{
    return FALSE;
}

NTSTATUS
SfFastIoAcquireForModeWrite (
                             IN struct _FILE_OBJECT *FileObject,
                             IN PLARGE_INTEGER EndingOffset,
                             OUT struct _ERESOURCE **ResourceToRelease,
                             IN struct _DEVICE_OBJECT *DeviceObject
                             )
{
    return 0;
}

NTSTATUS
SfFastIoReleaseForModWrite (
                            IN struct _FILE_OBJECT *FileObject,
                            IN struct _ERESOURCE *ResourceToRelease,
                            IN struct _DEVICE_OBJECT *DeviceObject
                            )
{
    return 0;
}

NTSTATUS
SfFastIoAcquireForCcFlush (
                           IN struct _FILE_OBJECT *FileObject,
                           IN struct _DEVICE_OBJECT *DeviceObject
                           )
{
    return 0;
}

NTSTATUS
SfFastIoReleaseForCcFlush (
                           IN struct _FILE_OBJECT *FileObject,
                           IN struct _DEVICE_OBJECT *DeviceObject
                           )
{
    return 0;
}

BOOLEAN
SfFastIoReadCompressed (
                        IN struct _FILE_OBJECT *FileObject,
                        IN PLARGE_INTEGER FileOffset,
                        IN ULONG Length,
                        IN ULONG LockKey,
                        OUT PVOID Buffer,
                        OUT PMDL *MdlChain,
                        OUT PIO_STATUS_BLOCK IoStatus,
                        OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
                        IN ULONG CompressedDataInfoLength,
                        IN struct _DEVICE_OBJECT *DeviceObject
                        )
{
    return FALSE;
}

BOOLEAN
SfFastIoWriteCompressed (
                         IN struct _FILE_OBJECT *FileObject,
                         IN PLARGE_INTEGER FileOffset,
                         IN ULONG Length,
                         IN ULONG LockKey,
                         IN PVOID Buffer,
                         OUT PMDL *MdlChain,
                         OUT PIO_STATUS_BLOCK IoStatus,
                         IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
                         IN ULONG CompressedDataInfoLength,
                         IN struct _DEVICE_OBJECT *DeviceObject
                         )
{
    return FALSE;
}

BOOLEAN
SfFastIoMdlReadCompleteCompressed (
                                   IN struct _FILE_OBJECT *FileObject,
                                   IN PMDL MdlChain,
                                   IN struct _DEVICE_OBJECT *DeviceObject
                                   )
{
    return FALSE;
}

BOOLEAN
SfFastIoMdlWriteCompleteCompressed (
                                    IN struct _FILE_OBJECT *FileObject,
                                    IN PLARGE_INTEGER FileOffset,
                                    IN PMDL MdlChain,
                                    IN struct _DEVICE_OBJECT *DeviceObject
                                    )
{
    return FALSE;
}

BOOLEAN
SfFastIoQueryOpen(
                  IN struct _IRP *Irp,
                  OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
                  IN struct _DEVICE_OBJECT *DeviceObject
                  )
{
    return FALSE;
}
// our own fastio structure for hooking fast I/O calls
BOOLEAN
InitFastIo(IN PDRIVER_OBJECT DriverObject)
{
    PFAST_IO_DISPATCH fastIoDispatch;
    fastIoDispatch = ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_IO_DISPATCH), SFLT_POOL_TAG);
    if (!fastIoDispatch) 
    {
        return FALSE;
    }
    // 内存清零。
    RtlZeroMemory(fastIoDispatch, sizeof(FAST_IO_DISPATCH));
    fastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
    //填写函数接口表
    fastIoDispatch->FastIoCheckIfPossible = SfFastIoCheckIfPossible;
    fastIoDispatch->FastIoRead = SfFastIoRead;
    fastIoDispatch->FastIoWrite = SfFastIoWrite;
    fastIoDispatch->FastIoQueryBasicInfo = SfFastIoQueryBasicInfo;
    fastIoDispatch->FastIoQueryStandardInfo = SfFastIoQueryStandardInfo;
    fastIoDispatch->FastIoLock = SfFastIoLock;
    fastIoDispatch->FastIoUnlockSingle = SfFastIoUnlockSingle;
    fastIoDispatch->FastIoUnlockAll = SfFastIoUnlockAll;
    fastIoDispatch->FastIoUnlockAllByKey = SfFastIoUnlockAllByKey;
    fastIoDispatch->FastIoDeviceControl = SfFastIoDeviceControl;
    fastIoDispatch->FastIoDetachDevice = SfFastIoDetachDevice;
    fastIoDispatch->FastIoQueryNetworkOpenInfo = SfFastIoQueryNetworkOpenInfo;
    fastIoDispatch->MdlRead = SfFastIoMdlRead;
    fastIoDispatch->MdlReadComplete = SfFastIoMdlReadComplete;
    fastIoDispatch->PrepareMdlWrite = SfFastIoPrepareMdlWrite;
    fastIoDispatch->MdlWriteComplete = SfFastIoMdlWriteComplete;
    fastIoDispatch->FastIoReadCompressed = SfFastIoReadCompressed;
    fastIoDispatch->FastIoWriteCompressed = SfFastIoWriteCompressed;
    fastIoDispatch->MdlReadCompleteCompressed = SfFastIoMdlReadCompleteCompressed;
    fastIoDispatch->MdlWriteCompleteCompressed = SfFastIoMdlWriteCompleteCompressed;
    fastIoDispatch->FastIoQueryOpen = SfFastIoQueryOpen;

    DriverObject->FastIoDispatch = fastIoDispatch;
    return TRUE;
}

VOID
DestoryFastIo(IN PDRIVER_OBJECT DriverObject)
{
    PFAST_IO_DISPATCH fastIoDispatch = DriverObject->FastIoDispatch;
    //PAGED_CODE();
    if (fastIoDispatch)
    {
        KdPrint(("FreeFastIo Pool\n"));
        DriverObject->FastIoDispatch = NULL;
        ExFreePool(fastIoDispatch);
    }
}
#endif