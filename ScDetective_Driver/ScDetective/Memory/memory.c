
#include "memory.h"

//////////////////////////////////////////////////////////////////////////

ULONG g_uCr0 = 0;

VOID WPOFF()
{
    ULONG uAttr;
    
    __asm {
        push    eax
        mov     eax, cr0
        mov     uAttr, eax
        and     eax, 0FFFEFFFFh
        mov     cr0, eax
        pop     eax
        cli
    }
    g_uCr0 = uAttr; 
}

VOID WPON()
{
    __asm {
        sti
        push    eax
        mov     eax, g_uCr0
        mov     cr0, eax
        pop     eax
    }
}

//////////////////////////////////////////////////////////////////////////

PVOID ScmMapVirtualAddress(PVOID VirtualAddress, ULONG Length, __out PMDL* MdlAddress)
{
    PVOID result = NULL;
    PMDL MdlAllocate = NULL;
    PMDL MdlNext = NULL;

    MdlAllocate = IoAllocateMdl(VirtualAddress, Length, FALSE, FALSE, NULL);
    MdlAddress[0] = MdlAllocate;

    if (MdlAllocate == NULL)  return result;

    MmProbeAndLockPages(MdlAllocate, KernelMode, IoModifyAccess);
    MdlNext = MdlAddress[0];

    if (MdlNext->MdlFlags & 5) {
        result = MdlNext->MappedSystemVa;
    } else {
        result = MmMapLockedPagesSpecifyCache(MdlNext, KernelMode, MmCached, NULL, 0, NormalPagePriority);
    }
    return result;
}

VOID ScmUnmapVirtualAddress(PMDL MdlAddress)
{
    ASSERT(MdlAddress);
    MmUnlockPages(MdlAddress);
    IoFreeMdl(MdlAddress);
}

//////////////////////////////////////////////////////////////////////////

ULONG ScmValidPage(ULONG address)
{
    ULONG   PAE_bit;
    ULONG   cr4;
    ULONG   pde_addr;
    ULONG   pte_addr;

    __asm   cli
    __asm {
        push eax

        _emit 0x0F
        _emit 0x20
        _emit 0xE0    //mov eax,cr4

        mov [cr4], eax
        pop eax
    }
    __asm   sti

    PAE_bit = cr4 & 0x20;

    if (PAE_bit != 0) {
        pde_addr = ((address >> 21) << 3) + 0xc0600000; 
    } else {
        pde_addr = ((address >> 22) << 2) + 0xc0300000;
    }

    if (*(PULONG)pde_addr & 0x1) {
        if (*(PULONG)pde_addr & 0x80) {
            return VALID_PAGE;
        } else {
            if (PAE_bit != 0) {
                pte_addr = ((address >> 12) << 3) + 0xc0000000;
            } else {
                pte_addr = ((address >> 12) << 2) + 0xc0000000;
            }
            if (*(PULONG)pte_addr & 0x1) {
                return VALID_PAGE;
            } else {
                return PTE_INVALID; 
            }
        }
    }  else { 
        return PDE_INVALID; 
    }
}

//////////////////////////////////////////////////////////////////////////

VOID InitializeMemoryValue(VOID)
{
    PDBGKD_GET_VERSION64 KdVersionBlock = NULL;
    PKDDEBUGGER_DATA64 DebuggerData = NULL;
    ULONG Time = 4;     // Only try 4 times

    // 多核 idt hook 也采用了类似的方法
_retry:
    KeSetSystemAffinityThread(1);
    __asm {
        mov     eax, fs:[0x1c]
        mov     eax, [eax+0x34]
        mov     KdVersionBlock, eax
    }
    KeRevertToUserAffinityThread();

    if (KdVersionBlock == NULL && 
        Time != 0)  {
        Time --;  goto _retry;
    } 
    if (KdVersionBlock != NULL) {
        DebuggerData = (PKDDEBUGGER_DATA64)*((PULONG)KdVersionBlock->DebuggerDataList);
        if (DebuggerData->Header.OwnerTag == KDBG_TAG) {
            ScmNonPagedPoolStart = *(PULONG)DebuggerData->MmNonPagedPoolStart;
            ScmNonPagedPoolEnd0G = ScmNonPagedPoolStart + 0x03000000;
        }
    } else {
        ScmNonPagedPoolStart = 0x80000000;
        ScmNonPagedPoolEnd0G = 0x90000000;
    }
    KdPrint(("[InitializeMemoryValue] MmNonPagedPoolStart : 0x%08x", ScmNonPagedPoolStart));
    KdPrint(("[InitializeMemoryValue] MmNonPagedPoolEnd0G : 0x%08x", ScmNonPagedPoolEnd0G));    
}

//////////////////////////////////////////////////////////////////////////