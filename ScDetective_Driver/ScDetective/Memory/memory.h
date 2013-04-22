
//////////////////////////////////////////////////////////////////////////
//
// Memory Operation
//
//////////////////////////////////////////////////////////////////////////

#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "ScDetective.h"

#define  INVALID_PAGE   0
#define  VALID_PAGE     1
#define  PDE_INVALID    2
#define  PTE_INVALID    3

#define PDE_SIZE        0x400000    //  4mb
#define PTE_SIZE        0x1000      //  4kb

//////////////////////////////////////////////////////////////////////////

ULONG ScmNonPagedPoolStart;
ULONG ScmNonPagedPoolEnd0G;     // Guess

//////////////////////////////////////////////////////////////////////////
PVOID 
ScmMapVirtualAddress(
    __in PVOID VirtualAddress, 
    __in ULONG Length, 
    __out PMDL* MdlAddress
    );

VOID 
ScmUnmapVirtualAddress(
    __in PMDL MdlAddress
    );

ULONG
ScmValidPage(
    ULONG   address
    );
VOID 
InitializeMemoryValue(
    VOID
    );

VOID WPOFF();
VOID WPON();

#endif