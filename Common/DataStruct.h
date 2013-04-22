
#ifndef _DATASTRUCT_H_
#define _DATASTRUCT_H_

#pragma pack(1)

typedef	struct _SSDT_NAME {
    ULONG   nIndex;
    char	FunName[64];
} SSDT_NAME, * PSSDT_NAME ; 

typedef	struct _SSDT_ADDRESS {
    ULONG	nIndex;
    DWORD	FunAddress;
} SSDT_ADDRESS, * PSSDT_ADDRESS ;

typedef struct _SSDT_INFO {
    ULONG   Index;
    char    FunName[64];
    DWORD   CurrentAddress;
    DWORD   NativeAddress;
    char    ImagePath[MAX_PATH];
    BOOL    Hideflag;
} SSDT_INFO, * PSSDT_INFO ;

typedef struct _PROCESS_INFO {
    LIST_ENTRY  ProcessLink;
    ULONG       UniqueProcessId;
    ULONG       InheritedProcessId;
    WCHAR       ImagePath[260];
    ULONG       EProcess;
    BOOL        bHidden;
} PROCESS_INFO, * PPROCESS_INFO ;

typedef struct _THREAD_INFO {
    LIST_ENTRY  ThreadLink;
    ULONG       ThreadId;
    ULONG       EThread;
    ULONG       Teb;
    signed char Priority;
    ULONG       Win32StartAddress;
    ULONG       ContextSwitches;
    WCHAR       State[4];       
} THREAD_INFO, * PTHREAD_INFO;

typedef struct _MODULE_INFO {
    LIST_ENTRY  ModuleLink;
    WCHAR       ImagePath[260];
    ULONG       BaseAddress;
    ULONG       ImageSize;
} MODULE_INFO, * PMODULE_INFO ;

typedef struct _DRIVER_INFO {
    LIST_ENTRY  DriverLink;
    ULONG       DriverObject;
    WCHAR       ImagePath[260];
    WCHAR       ServiceName[64];
    ULONG       ImageBase;
    ULONG       DriverSize;
    BOOL        bHidden;
} DRIVER_INFO, * PDRIVER_INFO ;

typedef struct _FILE_INFO {
    LIST_ENTRY      FileLink;
    WCHAR           FileName[128];
    ULONG           FileAttributes;
    LARGE_INTEGER   AllocationSize;
    LARGE_INTEGER   EndOfFile;
    TIME_FIELDS     CreationTime;
    TIME_FIELDS     LastWriteTime;
} FILE_INFO, * PFILE_INFO;

#pragma pack()

#endif