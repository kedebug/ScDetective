

#ifndef _SSDT_SHADOW_
#define _SSDT_SHADOW_

#include "ScDetective.h"

//////////////////////////////////////////////////////////////////////////

__declspec(dllimport) _stdcall KeAddSystemServiceTable(PVOID, PVOID, PVOID, PVOID, PVOID);

//////////////////////////////////////////////////////////////////////////
BOOLEAN
UnHookShadowSsdtItem(
    __in PSSDT_ADDRESS  AddressInfo,
    __in PEPROCESS      CsrssPEProcess
    );


ULONG
GetShadowSsdtServiceNumber(
    VOID
    );

ULONG 
GetShadowSsdtCurrentAddresses(
    PSSDT_ADDRESS   AddressInfoma, 
    PULONG          Length
    );


PSYSTEM_SERVICE_TABLE
GetKeServiceDescriptorTableShadow(
    VOID
    );

#endif