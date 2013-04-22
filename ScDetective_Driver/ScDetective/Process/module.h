
#ifndef _MODULE_H_
#define _MODULE_H_

#include "ScDetective.h"


typedef struct _OBJECT_CLASS {
    ULONG   MagicNumber;
    ULONG   Key;
    ULONG   NumberOfObject;
    USHORT  Type;
    USHORT  Size;
} OBJECT_CLASS, * POBJECT_CLASS ;

typedef struct _DRIVER_LIST_HEAD {
    LIST_ENTRY  DriverListHead;
    ULONG       NumberOfDrivers;
} DRIVER_LIST_HEAD, * PDRIVER_LIST_HEAD ;

typedef struct _DRIVER_CONTEXT {
    PDRIVER_OBJECT  DriverObject;
    PKEVENT         kEvent;
} DRIVER_CONTEXT, * PDRIVER_CONTEXT ;

//////////////////////////////////////////////////////////////////////////

PDRIVER_LIST_HEAD g_DriverListHead = NULL;

//////////////////////////////////////////////////////////////////////////

PDRIVER_LIST_HEAD 
ScObQueryDriverObject( 
    PDRIVER_OBJECT  DriverObject, 
    PKEVENT         UserEvent 
    );

ULONG
ExCopyDriverList2Buffer(
    PDRIVER_INFO    DriverInfo
    );

#endif
