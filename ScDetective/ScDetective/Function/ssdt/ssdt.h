
#ifndef _SSDT_H
#define _SSDT_H

#define	MOV_OPCODE	0xB8

;typedef struct _IMAGE_FIXUP_ENTRY {
    WORD    offset:12;
    WORD    type:4;
} IMAGE_FIXUP_ENTRY, * PIMAGE_FIXUP_ENTRY ;

DWORD
FindKiServiceTable(
    HMODULE hModule, 
    DWORD   dwKSDT
    );

PDWORD
GetSsdtNativeFunAddresses(
    PDWORD  NumOfAddress
    );

PSSDT_NAME 
GetSsdtNativeFunNames(
    PDWORD NumOfFunName
    );

//////////////////////////////////////////////////////////////////////////

PDWORD
GetShadowSsdtNativeFunAddresses(
    PDWORD NumberOfAddresses
    );

#endif

