
#include "stdafx.h"

VOID Convert2DosDeviceName(PTSTR pszSource, PTSTR pszDest, int cbszDest)
{
    WCHAR Buffer[3];
    WCHAR szName[520] = {0};

    if (pszSource == NULL)  return ;

    if (pszSource[0] != L'\\') 
    {
        wcscpy_s(pszDest, cbszDest, pszSource);
        return ;
    }

    for (int i = 'A'; i <= 'Z'; i ++)
    {
        Buffer[0] = (WCHAR)i;
        Buffer[1] = L':';
        Buffer[2] = L'\0';
        QueryDosDevice(Buffer, szName, _countof(szName));
        if (_wcsnicmp(szName, pszSource, 23) == 0)
        {
            wcscpy_s(pszDest, cbszDest, Buffer);
            wcscat_s(pszDest, cbszDest, pszSource + 23);
            return;
        }
    }
    // 若不成功，则使用设备路径
    lstrcpy(pszDest, pszSource);
}

VOID GetProcessImagePath(HANDLE hProcess, LPTSTR szFullImagePath)
{
    NTSTATUS ntStatus;
    PUNICODE_STRING NameString;
    ULONG ReturnLength;

    NameString = (PUNICODE_STRING)GlobalAlloc(GPTR, MAX_PATH * 2 + sizeof(UNICODE_STRING));
    NameString->Buffer = (PWSTR)((ULONG)NameString + sizeof(UNICODE_STRING));
    NameString->Length = 0;
    NameString->MaximumLength = MAX_PATH * 2;

    ntStatus = ZwQueryInformationProcess(hProcess, ProcessImageFileName,
                                         NameString, MAX_PATH * 2 + 8, &ReturnLength);
    if (NT_SUCCESS(ntStatus)) {
        RtlCopyMemory(szFullImagePath, NameString->Buffer, NameString->Length);
    }
    GlobalFree(NameString);
}

BOOL AquireUserAccess(ULONG ProcessId, PHANDLE TargetHandle)
{
    NTSTATUS ntStatus;
    BOOLEAN WasEnabled;
    PVOID Buffer = NULL;
    ULONG BufferSize = 0x10000;
    ULONG ReturnLength;
    HANDLE NeededHandle = INVALID_HANDLE_VALUE;
    HANDLE SourceHandle;
    HANDLE DuplicateHandle;
    BOOL ReturnValue = FALSE;
    PSYSTEM_HANDLE_INFORMATION Handles = NULL;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO HandleEntry = NULL;
    OBJECT_ATTRIBUTES ObAttributes = { sizeof(OBJECT_ATTRIBUTES), 0, NULL, NULL };
    CLIENT_ID ClientId;
    
    ASSERT(TargetHandle && ProcessId);

    RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    ClientId.UniqueProcess = (HANDLE)ProcessId;
    ClientId.UniqueThread  = 0;

    ntStatus = ZwOpenProcess(&NeededHandle, 
                             PROCESS_QUERY_INFORMATION, 
                             &ObAttributes, 
                             &ClientId);

    if (NT_SUCCESS(ntStatus)) {
        TargetHandle[0] = NeededHandle;
        return TRUE;
    }

_Retry:
    Buffer = GlobalAlloc(GPTR, BufferSize);
    if (Buffer == NULL)  return FALSE;
    ntStatus = ZwQuerySystemInformation(SystemHandleInformation,
                                        Buffer, 
                                        BufferSize, 
                                        &ReturnLength);

    if (ntStatus == STATUS_INFO_LENGTH_MISMATCH) {
        GlobalFree(Buffer);  BufferSize = ReturnLength;
        goto _Retry;
    }

    Handles = (PSYSTEM_HANDLE_INFORMATION)Buffer;
    HandleEntry = &(Handles->Handles[0]);

    for (ULONG i = 0; i < Handles->NumberOfHandles; i++, HandleEntry++)
    {
        if (HandleEntry->ObjectTypeIndex != OB_TYPE_PROCESS)  continue;

        ClientId.UniqueProcess = (HANDLE)HandleEntry->UniqueProcessId;
        ClientId.UniqueThread  = 0;

        ntStatus = ZwOpenProcess(&SourceHandle, 
                                 PROCESS_DUP_HANDLE, 
                                 &ObAttributes, 
                                 &ClientId);

        if (!NT_SUCCESS(ntStatus))  continue;

        ntStatus = ZwDuplicateObject(SourceHandle, 
                                     (HANDLE)HandleEntry->HandleValue,
                                     NtCurrentProcess(), 
                                     &DuplicateHandle, 
                                     PROCESS_ALL_ACCESS, 
                                     FALSE, 
                                     DUPLICATE_SAME_ACCESS);
        if (NT_SUCCESS(ntStatus)) {

            PROCESS_BASIC_INFORMATION BasicInfo = {0};

            ntStatus = ZwQueryInformationProcess(DuplicateHandle, 
                                                 ProcessBasicInformation,
                                                 &BasicInfo, 
                                                 sizeof(BasicInfo), 
                                                 NULL);
            if (NT_SUCCESS(ntStatus)) {

                if (BasicInfo.UniqueProcessId == ProcessId) {

                    ntStatus = ZwDuplicateObject(SourceHandle, 
                                                 (HANDLE)HandleEntry->HandleValue,
                                                 NtCurrentProcess(), 
                                                 &NeededHandle, 
                                                 PROCESS_QUERY_INFORMATION, 
                                                 FALSE, 0);
                    ZwClose(SourceHandle);
                    ZwClose(DuplicateHandle);

                    if (NT_SUCCESS(ntStatus))  ReturnValue = TRUE;
                    break;
                }
            }
            ZwClose(DuplicateHandle);
        }
        ZwClose(SourceHandle);
    }
    
    if (Buffer)  GlobalFree(Buffer);

    if (ReturnValue == FALSE) {
        TargetHandle[0] = INVALID_HANDLE_VALUE;         
    } else {
        TargetHandle[0] = NeededHandle;
    }
    return ReturnValue;
}

VOID CloseMyHandles(ULONG ProcessId)
{
    NTSTATUS ntStatus;
    BOOLEAN wasEnabled;
    PVOID Buffer = NULL;
    DWORD BufferSize = 0x10000, NeedSize = 0;
    HANDLE TargetProcess = INVALID_HANDLE_VALUE;
    HANDLE hOwnerProcess;
    HANDLE hProcess;
    PSYSTEM_HANDLE_INFORMATION Handles = NULL;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO HandleEntry = NULL;
    OBJECT_ATTRIBUTES objatr = {sizeof(OBJECT_ATTRIBUTES), 0, NULL, NULL};
    CLIENT_ID ClientId;

    RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &wasEnabled);

    ClientId.UniqueProcess = (HANDLE)ProcessId;
    ClientId.UniqueThread  = 0;

_Retry:
    Buffer = GlobalAlloc(GPTR, BufferSize);
    if (Buffer == NULL)  return ;
    ntStatus = ZwQuerySystemInformation(SystemHandleInformation,
                                        Buffer, 
                                        BufferSize, 
                                        &NeedSize);

    if (ntStatus == STATUS_INFO_LENGTH_MISMATCH) {
        GlobalFree(Buffer);
        BufferSize = NeedSize;
        goto _Retry;
    }

    Handles = (PSYSTEM_HANDLE_INFORMATION)Buffer;
    HandleEntry = &(Handles->Handles[0]);

    for (ULONG i = 0; i < Handles->NumberOfHandles; i++, HandleEntry++)
    {
        if (HandleEntry->ObjectTypeIndex != OB_TYPE_PROCESS)  continue;

        ClientId.UniqueProcess = (HANDLE)HandleEntry->UniqueProcessId;
        ClientId.UniqueThread  = 0;

        ntStatus = ZwOpenProcess(&hOwnerProcess, 
                                 PROCESS_DUP_HANDLE, 
                                 &objatr, 
                                 &ClientId);

        if (!NT_SUCCESS(ntStatus))  continue;

        ntStatus = ZwDuplicateObject(hOwnerProcess, 
                                     (HANDLE)HandleEntry->HandleValue,
                                     NtCurrentProcess(),
                                     &hProcess, 
                                     PROCESS_ALL_ACCESS, 
                                     FALSE, 
                                     DUPLICATE_SAME_ACCESS);

        if (NT_SUCCESS(ntStatus)) {

            PROCESS_BASIC_INFORMATION BasicInfo = {0};
            ntStatus = ZwQueryInformationProcess(hProcess, 
                                                 ProcessBasicInformation,
                                                 &BasicInfo, 
                                                 sizeof(BasicInfo), 
                                                 NULL);
            if (NT_SUCCESS(ntStatus)) {

                if (BasicInfo.UniqueProcessId == ProcessId) {

                    ntStatus = ZwDuplicateObject(hOwnerProcess, 
                                                 (HANDLE)HandleEntry->HandleValue,
                                                 NtCurrentProcess(), 
                                                 &TargetProcess, 
                                                 PROCESS_ALL_ACCESS, 
                                                 FALSE, 
                                                 DUPLICATE_CLOSE_SOURCE);

                    if (NT_SUCCESS(ntStatus))  ZwClose(TargetProcess);
                }
            }
            ZwClose(hProcess);
        }
        ZwClose(hOwnerProcess);
    }
    if (Buffer)  GlobalFree(Buffer);
}