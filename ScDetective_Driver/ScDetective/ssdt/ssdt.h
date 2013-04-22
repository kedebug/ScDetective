
#ifndef __SSDT_H__
#define __SSDT_H__

#include "ScDetective.h"

#define	MOV_OPCODE	0xB8

//////////////////////////////////////////////////////////////////////////

__declspec(dllimport) SYSTEM_SERVICE_TABLE KeServiceDescriptorTable;

//////////////////////////////////////////////////////////////////////////

ULONG
GetSsdtServiceNumber(
    );

ULONG
GetSsdtCurrentAddresses(
    IN PSSDT_ADDRESS    AddressInfo,
    OUT IN PULONG       Length
    );

BOOLEAN
UnHookSsdtItem(
    IN PSSDT_ADDRESS SsdtInfo
    );

ULONG 
SetServiceAddress( 
    IN UINT     ServiceIndex, 
    IN ULONG    NewServiceAddress 
    );

ULONG
GetServiceIdByName(
    PCHAR   FunctionName
    );

#endif