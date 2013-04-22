
#include "ssdt.h"


///////////////////////////////////////////////////////////////////////////////////
//
//	功能实现：获取SSDT表中的服务个数
//	输入参数：无
//	输出参数：返回SSDT表中的服务个数
//
///////////////////////////////////////////////////////////////////////////////////
ULONG 
GetSsdtServiceNumber()
{
    KdPrint(("Enter GetSsdtServiceNumber"));
    return KeServiceDescriptorTable.NumberOfService;
}

///////////////////////////////////////////////////////////////////////////////////
//
//	功能实现：枚举SSDT表中的服务函数地址
//	输入参数：AddressInfo 为类型为 PSSDT_INFO的输出缓冲区指针,当其为NULL时，Length中带回所需要的缓冲区长度；
//			 Length为输出缓冲区长度指针，在函数返回时，改参数返回写入缓冲区的数据长度
//	输出参数：返回SSDT表中的服务函数个数
//
///////////////////////////////////////////////////////////////////////////////////
ULONG 
GetSsdtCurrentAddresses (
    IN PSSDT_ADDRESS AddressInfo, 
    OUT IN PULONG Length
    )
{
    ULONG   ServiceNumber;
    ULONG   index;

    ServiceNumber = KeServiceDescriptorTable.NumberOfService;

    if (ServiceNumber * sizeof(SSDT_ADDRESS) > *Length)
    {
        *Length = ServiceNumber * sizeof(SSDT_ADDRESS);
        return 0;
    }

    if (AddressInfo == NULL)
    {
        *Length = ServiceNumber * sizeof(SSDT_ADDRESS);
        return 0;
    }

    for (index = 0; index < ServiceNumber; index++)
    {
        AddressInfo[index].nIndex = index;
        AddressInfo[index].FunAddress = (DWORD)KeServiceDescriptorTable.ServiceTableBase[index];
    }

    *Length = ServiceNumber * sizeof(SSDT_ADDRESS);
    return  ServiceNumber;
}

///////////////////////////////////////////////////////////////////////////////////
//
//	功能实现： 修改SSDT表中指定的服务函数地址,可以用于SSDT表服务函数HOOK
//	输入参数： ServiceIndex为要修改的服务函数在SSDT表中的序数；
//			  NewServiceAddress 为要修改的服务函数的新地址；
//	输出参数： 返回指定的被修改服务函数的原始地址
//
///////////////////////////////////////////////////////////////////////////////////
ULONG
SetServiceAddress(
    IN UINT     ServiceIndex,
    IN ULONG    NewServiceAddress
    )
{
    PMDL    MdlSystemCall = NULL;
    PVOID*  MappedPointer = NULL;
    PULONG  EnabledAddress = NULL;
    ULONG   OrigServiceAddress = 0;
    KIRQL   OrigIrql;
    
    KdPrint(("Enter SetServiceAddress"));
/*
    MdlSystemCall = MmCreateMdl(NULL, KeServiceDescriptorTable.ServiceTableBase, 
                                KeServiceDescriptorTable.NumberOfService * sizeof(ULONG)
                                );
*/
    MdlSystemCall = IoAllocateMdl(KeServiceDescriptorTable.ServiceTableBase,
                                  KeServiceDescriptorTable.NumberOfService * sizeof(ULONG),
                                  FALSE, FALSE, NULL);
    if (MdlSystemCall == NULL)
    {
        KdPrint(("[SetServiceAddress] MmCreateMdl failed"));
        return OrigServiceAddress;
    }

    MmBuildMdlForNonPagedPool(MdlSystemCall);
    MdlSystemCall->MdlFlags = MdlSystemCall->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;
    MappedPointer = MmMapLockedPagesSpecifyCache(MdlSystemCall, KernelMode, MmCached,
                                                 NULL, FALSE, NormalPagePriority );
    EnabledAddress = (PULONG)MappedPointer;

    KeRaiseIrql(DISPATCH_LEVEL, &OrigIrql);
    OrigServiceAddress = EnabledAddress[ServiceIndex];
    EnabledAddress[ServiceIndex] = NewServiceAddress;
    KeLowerIrql(OrigIrql);

    MmUnmapLockedPages(MappedPointer, MdlSystemCall);
    IoFreeMdl(MdlSystemCall);
    return OrigServiceAddress;
}

///////////////////////////////////////////////////////////////////////////////////
//
//	功能实现：恢复SSDT表中指定序数的服务函数的地址
//	输入参数：SsdtInfo 为 SSDT_ADDRESS 类型的指针，里面是要恢复的服务函数序数和地址
//	输出参数：返回TRUE表示恢复成功，否则返回FALSE失败
//
///////////////////////////////////////////////////////////////////////////////////
BOOLEAN
UnHookSsdtItem(
    IN PSSDT_ADDRESS SsdtInfo
    )
{
    if (SsdtInfo == NULL)
    {
        KdPrint(("[UnHookSsdtItem] 输入指针无效"));
        return FALSE;
    }

    if (SetServiceAddress(SsdtInfo->nIndex, SsdtInfo->FunAddress))
    {
        KdPrint(("[UnHookSsdtItem] 恢复 %d 号服务成功", SsdtInfo->nIndex));
        return TRUE;
    }
    else
    {
        KdPrint(("[UnHookSsdtItem] SetServiceAddress failed"));
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////

ULONG GetServiceIdByName(PCHAR FunctionName)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    ULONG ServiceId = 0;
    UNICODE_STRING RootName;
    PUNICODE_STRING KernelLinkName = NULL;
    HANDLE FileHandle, SectionHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK ioStatus;
    PVOID BaseAddress = NULL;
    size_t ViewSize = 0;
    PIMAGE_DOS_HEADER pDosHeader = NULL;
    PIMAGE_NT_HEADERS pNtHeader = NULL;
    PIMAGE_OPTIONAL_HEADER pOptionalHeader = NULL;
    PIMAGE_EXPORT_DIRECTORY pExportDirectory = NULL;
    PULONG arrayOfFunctionNames = NULL;
    PULONG arrayOfFunctionAddresses = NULL;
    PWORD  arrayOfFunctionOrdinals = NULL;
    ULONG  i, funcOrdinal, funcAddress, number;
    PCHAR  funcName = NULL;

    RtlInitUnicodeString(&RootName, L"\\SystemRoot");
    KernelLinkName = Convert2KernelLinkName(&RootName);
    if (KernelLinkName == NULL)  return 0;
    
    ntStatus = RtlAppendUnicodeToString(KernelLinkName, L"\\system32\\ntdll.dll");
    if (!NT_SUCCESS(ntStatus)) {
        ExFreePoolWithTag(KernelLinkName, MEM_TAG);
        return 0;
    }
    InitializeObjectAttributes(&ObjectAttributes, KernelLinkName, 
                               OBJ_CASE_INSENSITIVE, 0, 0);

    ntStatus = ZwOpenFile(&FileHandle, FILE_EXECUTE | SYNCHRONIZE, 
                          &ObjectAttributes, &ioStatus, 
                          FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(ntStatus))  return 0;

    ObjectAttributes.ObjectName = NULL;

    ntStatus = ZwCreateSection(&SectionHandle, SECTION_ALL_ACCESS, 
                               &ObjectAttributes, 0, 
                               PAGE_EXECUTE, SEC_IMAGE, FileHandle);
    if (!NT_SUCCESS(ntStatus)) { 
        ZwClose(FileHandle); 
        return 0; 
    }
    ntStatus = ZwMapViewOfSection(SectionHandle, NtCurrentProcess(), 
                                  &BaseAddress, 0, 1024, 0, &ViewSize, 
                                  ViewShare, MEM_TOP_DOWN, PAGE_READWRITE);
    if (!NT_SUCCESS(ntStatus)) { 
        ZwClose(SectionHandle); 
        ZwClose(FileHandle); 
        return 0; 
    }
    pDosHeader = (PIMAGE_DOS_HEADER)BaseAddress;
    pNtHeader  = (PIMAGE_NT_HEADERS)((ULONG)BaseAddress + pDosHeader->e_lfanew);
    pOptionalHeader = &(pNtHeader->OptionalHeader);
    pExportDirectory = (PIMAGE_EXPORT_DIRECTORY)(pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + (ULONG)BaseAddress);

    arrayOfFunctionNames = (PULONG)(pExportDirectory->AddressOfNames + (ULONG)BaseAddress);
    arrayOfFunctionAddresses = (PULONG)(pExportDirectory->AddressOfFunctions + (ULONG)BaseAddress);
    arrayOfFunctionOrdinals = (PWORD)(pExportDirectory->AddressOfNameOrdinals + (ULONG)BaseAddress);

    for (i = 0; i < pExportDirectory->NumberOfNames; i++) {

        funcName = (PCHAR)(arrayOfFunctionNames[i] + (PCHAR)BaseAddress);
        funcOrdinal = arrayOfFunctionOrdinals[i] + pExportDirectory->Base - 1;
        funcAddress = (ULONG)(arrayOfFunctionAddresses[funcOrdinal] + (PCHAR)BaseAddress);

        if (funcName[0] == 'N' && funcName[1] == 't') {

            number = *((PULONG)(funcAddress + 1));
            if (*(PBYTE)funcAddress != MOV_OPCODE)  continue;
            if (number > pExportDirectory->NumberOfNames)  continue;

            if (strstr(funcName, FunctionName) != 0) {

                ServiceId = number;  break;
            }
        }
    }
    ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
    ZwClose(SectionHandle);
    ZwClose(FileHandle);
    ExFreePoolWithTag(KernelLinkName, MEM_TAG);
    return ServiceId;
}

////////////////////// End of File ///////////////////////////////////
