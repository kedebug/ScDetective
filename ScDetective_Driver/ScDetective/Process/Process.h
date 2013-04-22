
#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "ScDetective.h"

#define OBJECT_HEADER_SIZE      0x018
#define OBJECT_TYPE_OFFSET      0x008

WCHAR ThreadState[9][4] = { L"Ô¤ÖÃ", L"¾ÍÐ÷", L"ÔËÐÐ", L"´ýÃü", 
                            L"ÖÕÖ¹", L"µÈ´ý", L"ÇÐ»»", L"-", L"-" };

typedef struct _THREAD_CONTEXT {
    PEPROCESS   Process;
    KEVENT      SynEvent;
} THREAD_CONTEXT, * PTHREAD_CONTEXT ;

typedef struct _PROCESS_LIST_HEAD {
    LIST_ENTRY  ProcessListHead;
    ULONG       NumberOfProcesses;
} PROCESS_LIST_HEAD, * PPROCESS_LIST_HEAD ;

typedef struct _THREAD_LIST_HEAD {
    LIST_ENTRY  ThreadListHead;
    ULONG       NumberOfThread;
} THREAD_LIST_HEAD, * PTHREAD_LIST_HEAD;

typedef struct _MODULE_LIST_HEAD {
    LIST_ENTRY  ModuleListHead;
    ULONG       NumberOfModules;
} MODULE_LIST_HEAD, * PMODULE_LIST_HEAD;

//////////////////////////////////////////////////////////////////////////

PPROCESS_LIST_HEAD  g_ProcessListHead = NULL;
PTHREAD_LIST_HEAD   g_ThreadListHead = NULL;
PMODULE_LIST_HEAD   g_ModuleListHead = NULL;

//////////////////////////////////////////////////////////////////////////

typedef PVOID (NTAPI * pFnObGetObjectType)(IN PVOID pObject);

typedef NTSTATUS (NTAPI * pFnZwQueryVirtualMemory) (
                  __in HANDLE ProcessHandle,
                  __in PVOID BaseAddress,
                  __in MEMORY_INFORMATION_CLASS MemoryInformationClass,
                  __out_bcount(MemoryInformationLength) PVOID MemoryInformation,
                  __in SIZE_T MemoryInformationLength,
                  __out_opt PSIZE_T ReturnLength
                  );

//////////////////////////////////////////////////////////////////////////

ULONG
ScObGetObjectType(
    PVOID Object
    );

PPROCESS_LIST_HEAD 
ScPsQuerySystemProcessList(
    VOID
    );

PTHREAD_LIST_HEAD
ScPsQueryProcessThreadList(
    __in PEPROCESS  EProcess
    );

PMODULE_LIST_HEAD 
ScPsQueryProcessModuleList(
    __in PEPROCESS  EProcess
    );

ULONG 
ExCopyModuleList2Buffer(
    __in PMODULE_INFO    ModuleInfo
    );

ULONG 
ExCopyProcessList2Buffer(
    __in PPROCESS_INFO   ProcessInfo
    );

ULONG 
ExCopyThreadList2Buffer(
    __in PTHREAD_INFO ThreadInfo
    );

NTSTATUS
ScPsGetProcessImagePath(
    __in PEPROCESS Process,
    __out PUNICODE_STRING NameString
    );

PEPROCESS 
ScPsGetThreadProcess(
    __in PETHREAD Thread
    );

PEPROCESS 
ScPsGetSystemIdleProcess(
    VOID
    );

NTSTATUS 
ScPsLookupProcessByName(
    PEPROCESS* TargetProcess, 
    PWCH pszImageName
    );

ULONG 
ScPsGetCsrssProcessId(
    VOID
    );

ULONG 
ScPsGetPspCidTable(
    VOID
    );

ULONG 
ScPsGetPsActiveProcessHead(
    VOID
    );

#endif