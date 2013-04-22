
#ifndef _INTI_H_
#define _INIT_H_

#include "ScDetective.h"

#define WINDOWS_VERSION_NONE            0L       // 0
#define WINDOWS_VERSION_2K              1L
#define WINDOWS_VERSION_XP              2L
#define WINDOWS_VERSION_2K3             3L
#define WINDOWS_VERSION_2K3_SP1_SP2     4L
#define WINDOWS_VERSION_VISTA           5L  
#define WINDOWS_VERSION_7               6L

ULONG g_WindowsVersion;

ULONG g_WindowsBuildNumber;

//
// EPROCESS
//
ULONG offset_Process_CreateTime;
ULONG offset_Process_ExitTime;
ULONG offset_Process_ActiveProcessLinks;
ULONG offset_Process_UniqueProcessId;
ULONG offset_Process_ImageFileName;
ULONG offset_Process_ThreadListHead;
ULONG offset_Process_ObjectTable;
ULONG offset_Process_Peb;
ULONG offset_Process_InheritedProcessId;
ULONG g_EPROCESS_ActiveThreads;
ULONG g_EPROCESS_JobLinks;
ULONG offset_Process_SectionObject;    
ULONG EPROCESS_SIZE;

//
// EThread
//
ULONG offset_Thread_Cid;
ULONG offset_Thread_StartAddress;
ULONG offset_Thread_ThreadListEntry;
ULONG offset_Thread_Teb;   
ULONG offset_Thread_ThreadsProcess;
ULONG offset_Thread_ApcState;
ULONG g_ETHREAD_CreateTime;
ULONG g_ETHREAD_ExitTime;
ULONG offset_Thread_ApcStateIndex;
ULONG offset_Thread_SavedApcState;
ULONG offset_Thread_State;
ULONG offset_Thread_Priority;
ULONG offset_Thread_ContextSwitches;
ULONG offset_Thread_Win32StartAddress;
ULONG g_ETHREAD_ApcQueueable;
ULONG g_ETHREAD_Alertable;

ULONG g_PEB_ProcessParameters;
ULONG g_PEB_Ldr;

//
// _LDR_DATA_TABLE_ENTRY
//
ULONG offset_LdrData_InLoadOrderModuleList;
ULONG offset_LdrData_DLLBase;
ULONG offset_LdrData_EntryPoint;
ULONG offset_LdrData_SizeOfImage; 
ULONG offset_LdrData_FullDllName;
ULONG offset_LdrData_BaseDllName;

ULONG g_PROCESS_PARAMETERS_ImagePathName;

ULONG g_EJOB_ActiveProcesses;
ULONG g_EJOB_ProcessListHead;

//
// KPCR
// FS寄存器指向KPCR(Kernel's Processor Control Region)结构
// FS:[0x120]处就是KPRCB（Kernel's Processor Cotrol Block）结构
//
ULONG offset_kpcr_IdleThread;

//
// _OBJECT_TYPE
//
ULONG offset_ObjectType_Key;
ULONG offset_ObjectType_TotalNumberOfObjects;

// 
// _HANDLE_TABLE
// 
ULONG offset_HandleTable_HandleTableList;
ULONG offset_HandleTable_QuotaProcess;

ULONG ServiceId_NtQueryVirtualMemory;
ULONG ServiceId_NtCreateThread;
ULONG ServiceId_NtTerminateThread;

NTSTATUS
GetWindowsVersion(
    VOID
    );

NTSTATUS
InitializeOffsetByOsVersion(
    VOID
    );

NTSTATUS
InitializeScDetective(
    VOID
    );

#endif