
#ifndef __HOOK_ENGINE__
#define __HOOK_ENGINE__

#include "ScDetective.h"

#define PROCESS_TERMINATE   0x0001
//////////////////////////////////////////////////////////////////////////

typedef struct _ADDRESS_TABLE {
    ULONG   pFnZwOpenKey;
    ULONG   pFnZwClose;
    ULONG   pFnZwQueryValueKey;
    ULONG   pFnZwDeleteKey;
    ULONG   pFnZwSetValueKey;
    ULONG   pFnZwCreateKey;
    ULONG   pFnZwDeleteValueKey; 
    ULONG   pFnZwEnumValueKey;
    ULONG   pFnZwRestoreKey;
    ULONG   pFnZwReplaceKey;
    ULONG   pFnZwTerminateProcess;
    ULONG   pFnZwDuplicateObject; 
    ULONG   pFnZwCreateThread;
    ULONG   pFnZwTerminateThread;
    ULONG   pFnZwSetSystemInformation;
    // H
    ULONG   pFnKeInsertQueneApc;
} ADDRESS_TABLE, * PADDRESS_TABLE;

typedef struct _DPC_CONTEXT {
    PKDPC   Dpcs;
    ULONG   LockedProcessors;
    BOOL    ReleaseFlag;
} DPC_CONTEXT, * PDPC_CONTEXT ;

//////////////////////////////////////////////////////////////////////////

#define PREPARE_HOOK_NUMBER         sizeof(ADDRESS_TABLE) / sizeof(ULONG)

#define SYSCALL_INDEX(_Function)    *(PULONG)((PUCHAR)_Function + 1)

#define SYSCALL_ADDRESS(_Function)  \
    KeServiceDescriptorTable.ServiceTableBase[SYSCALL_INDEX(_Function)]

//////////////////////////////////////////////////////////////////////////

#pragma code_seg()

BOOL  bAlreadyHooked = FALSE;

// 需要 hook 的函数数量
ULONG NumberOfHookedFunction;

// 用于标识函数是否被 hook
UCHAR HookFlags[PREPARE_HOOK_NUMBER];

// 用于表示被覆盖的其实地址地址
ULONG CoverStartAddress[PREPARE_HOOK_NUMBER];

// 用于标识函数被覆盖的字节数
ULONG CoverLength[PREPARE_HOOK_NUMBER];

// 用于存放函数被覆盖的字节, 为每个函数提供 32 个字节
UCHAR OrigOpCode[PREPARE_HOOK_NUMBER][32];

// 原始函数地址数组
ADDRESS_TABLE OrigAddressTable;

// 自己提供的函数地址数组
ADDRESS_TABLE fakeAddressTable;

//////////////////////////////////////////////////////////////////////////
typedef
NTSTATUS  (* pFnZwClose) (
                    __in HANDLE Handle
                    );
typedef 
KAFFINITY (* pFnKeSetAffinityThread) (
                    __inout PKTHREAD Thread,
                    __in KAFFINITY Affinity
                    );

typedef VOID  (FASTCALL *pFnKiInsertQueueApc)(
                    __in PKAPC Apc,
                    __in KPRIORITY 
                    Increment
                    );

pFnKiInsertQueueApc g_OrigKiInsertQueueApc;

BOOL 
ScHeSafeInlineHook(
    __in PVOID TargetAddress, 
    __in PVOID ReadyOpCode, 
    __in ULONG OpCodeLength
    );

VOID 
InitilizeHook(
    VOID
    );
VOID 
UnInlineHookNativeApi(
    VOID
    );

#endif

//
// 2011/4/28添加 KiInsertQueueApc 保护，个人感觉很挫，通用性不好
//