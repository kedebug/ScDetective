
#ifndef __SC_PROTECT__
#define __SC_PROTECT__

#include "ScDetective.h"

//////////////////////////////////////////////////////////////////////////

typedef struct _HANDLE_TABLE_ENTRY {
    union {
        PVOID Object;
        ULONG ObAttributes;
    };
    union {
        union {
            ACCESS_MASK GrantedAccess;
            struct {
                USHORT GrantedAccessIndex;
                USHORT CreatorBackTraceIndex;
            };
        };
        LONG NextFreeTableEntry;
    };
} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

//////////////////////////////////////////////////////////////////////////

typedef BOOLEAN (* EX_ENUMERATE_HANDLE_ROUTINE)(
                    IN PHANDLE_TABLE_ENTRY HandleTableEntry,
                    IN HANDLE Handle,
                    IN PVOID EnumParameter
                    );

typedef BOOLEAN (* pFnExEnumHandleTable)(
	                IN PVOID HandleTable,
	                IN EX_ENUMERATE_HANDLE_ROUTINE EnumHandleProcedure,
	                IN PVOID EnumParameter,
	                OUT PHANDLE Handle OPTIONAL
	                );

//////////////////////////////////////////////////////////////////////////

ULONG EraseNumber = 0;

typedef struct _PROTECT_INFO {
    HANDLE_TABLE_ENTRY ObjectInfo[16];
    PLIST_ENTRY ActiveProcessList;
    PVOID ObjectEntry[16];
    HANDLE HiddenProcessId;
} PROTECT_INFO, * PPROTECT_INFO ;

PROTECT_INFO pdoHideGlobalInfo;

//////////////////////////////////////////////////////////////////////////

NTSTATUS
ScPtHideProcessById(
    __in HANDLE  ProcessId
    );

VOID
ScPtUnloadRoutine(
    VOID
    );
#endif