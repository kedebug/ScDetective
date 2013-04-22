
// 使用预编译头
#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////////////
//
//	功能实现：查找KiServiceTable
//	输入参数：hModule为模块加载地址，
//			  dwKSDT为KeServiceDescriptorTable RVA
//	输出参数：返回KiServiceTable的偏移（RVA）
//
///////////////////////////////////////////////////////////////////////////////////
DWORD 
FindKiServiceTable(HMODULE hModule, DWORD dwKSDT)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS pNtHeader  = (PIMAGE_NT_HEADERS)((DWORD)hModule + pDosHeader->e_lfanew);
    PIMAGE_FILE_HEADER pFileHeader = &(pNtHeader->FileHeader);
    PIMAGE_OPTIONAL_HEADER pOptionalHeader = &(pNtHeader->OptionalHeader);
    PIMAGE_BASE_RELOCATION pBaseReloc;
    PIMAGE_FIXUP_ENTRY pFixupEntry;

    if (pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress &&
        ! ((pFileHeader->Characteristics) & IMAGE_FILE_RELOCS_STRIPPED))
    {
        pBaseReloc = (PIMAGE_BASE_RELOCATION)(pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + (DWORD)hModule);

        BOOL bFirstChunk = TRUE;
        // first IMAGE_BASE_RELOCATION.VirtualAddress of ntoskrnl is 0
        while (bFirstChunk || pBaseReloc->VirtualAddress)
        {
            bFirstChunk = FALSE;

            pFixupEntry = (PIMAGE_FIXUP_ENTRY)((DWORD)pBaseReloc + sizeof(IMAGE_BASE_RELOCATION));

            UINT nNumofRelocItem = pBaseReloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION);
            nNumofRelocItem = nNumofRelocItem >> 1;
            for (UINT i = 0; i < nNumofRelocItem; i++, pFixupEntry++)
            {
                if (pFixupEntry->type != IMAGE_REL_BASED_HIGHLOW) continue;

                DWORD dwPointerRva = pBaseReloc->VirtualAddress + pFixupEntry->offset;
                DWORD dwPointsToRva = *(PDWORD)((DWORD)hModule + dwPointerRva) - (DWORD)pOptionalHeader->ImageBase;

                if (dwPointsToRva == dwKSDT)
                {
                    if (*(PWORD)((DWORD)hModule + dwPointerRva - 2) == 0x05c7)
                    {
                        return *(PDWORD)((DWORD)hModule + dwPointerRva + 4) - pOptionalHeader->ImageBase;
                    }
                }                
            }
            *(PDWORD)&pBaseReloc += pBaseReloc->SizeOfBlock;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////
//
//	功能实现：解析内核文件，获取SSDT表中的服务函数原始地址
//	输入参数：*NumOfAddress返回原始SSDT表中的服务函数个数
//	输出参数：返回DWORD类型的地址数组指针
//	注释：返回的缓冲区最终要用 GlobalFree 释放
//
///////////////////////////////////////////////////////////////////////////////////
PDWORD 
GetSsdtNativeFunAddresses(PDWORD  NumOfAddress)
{
    DWORD dwKiServiceTable;
    DWORD KernelBase, dwKSDT;
    BOOL bStatus = FALSE;
    char szKernelName[16] = { 0 };
    
    bStatus = GetKernelInformation(szKernelName, 16, &KernelBase);
    if (bStatus == FALSE)   return NULL;

    HMODULE hKernel = LoadLibraryExA(szKernelName, 0, DONT_RESOLVE_DLL_REFERENCES);
    if (!hKernel)   return NULL;

    dwKSDT = (DWORD)GetProcAddress(hKernel, "KeServiceDescriptorTable");
    if (!dwKSDT)  
    {
        FreeLibrary(hKernel);
        return NULL;
    }

    // Get KeServiceDescriptorTable RVA
    dwKSDT -= (DWORD)hKernel;
    dwKiServiceTable = FindKiServiceTable(hKernel, dwKSDT);
    if (!dwKiServiceTable)   
    {
        FreeLibrary(hKernel);
        return NULL;
    }

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hKernel;
    PIMAGE_NT_HEADERS pNtHeader  = (PIMAGE_NT_HEADERS)((DWORD)hKernel + pDosHeader->e_lfanew);
    PIMAGE_OPTIONAL_HEADER pOptionalHeader = &(pNtHeader->OptionalHeader);
    DWORD dwServices = 0;

    for (PDWORD pService = (PDWORD)((DWORD)hKernel + dwKiServiceTable);
        *pService - pOptionalHeader->ImageBase < pOptionalHeader->SizeOfImage;
        pService ++, dwServices ++)
    {
        // Only to get dwServices
    }

    PDWORD pBuffer = NULL;
    pBuffer = (PDWORD)GlobalAlloc(GPTR, dwServices * 4 + 4);
    if (pBuffer == NULL)  return NULL;

    *NumOfAddress = dwServices;
    dwServices = 0;
    for (PDWORD pService = (PDWORD)((DWORD)hKernel + dwKiServiceTable);
         *pService - pOptionalHeader->ImageBase < pOptionalHeader->SizeOfImage;
         pService ++, dwServices ++)
    {
        pBuffer[dwServices] = *pService - pOptionalHeader->ImageBase + KernelBase;
    }

    FreeLibrary(hKernel);
    return pBuffer;
}

//////////////////////////////////////////////////////////////////////////////////
//
//	功能实现：解析内核文件，获取SSDT表中的服务函数原始地址
//	输入参数：*NumOfFunName返回原始SSDT表中的服务函数个数
//	输出参数：返回DWORD类型的地址数组指针
//	注释：返回的缓冲区最终要用 GlobalFree 释放
//
///////////////////////////////////////////////////////////////////////////////////
PSSDT_NAME 
GetSsdtNativeFunNames(PDWORD NumOfFunName)
{
    HMODULE hNtdll = GetModuleHandle(L"ntdll.dll");
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hNtdll;
    PIMAGE_NT_HEADERS pNtHeader  = (PIMAGE_NT_HEADERS)((ULONG)hNtdll + pDosHeader->e_lfanew);
    PIMAGE_OPTIONAL_HEADER pOptionalHeader = &(pNtHeader->OptionalHeader);
    PIMAGE_EXPORT_DIRECTORY pExportDirectory = (PIMAGE_EXPORT_DIRECTORY)(pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + (DWORD)hNtdll);

    PDWORD arrayOfFunctionNames = (PDWORD)(pExportDirectory->AddressOfNames + (DWORD)hNtdll);
    PDWORD arrayOfFunctionAddresses = (PDWORD)(pExportDirectory->AddressOfFunctions + (DWORD)hNtdll);
    PWORD  arrayOfFunctionOrdinals = (PWORD)(pExportDirectory->AddressOfNameOrdinals + (DWORD)hNtdll);
    
    DWORD dwCount = 0;
    PSSDT_NAME pSsdtName = NULL;
    for (UINT i = 0; i < pExportDirectory->NumberOfNames; i++)
    {
        char* funcName = (char*)(arrayOfFunctionNames[i] + (PCHAR)hNtdll);
        DWORD funcOrdinal = arrayOfFunctionOrdinals[i] + pExportDirectory->Base - 1;
        DWORD funcAddress = (DWORD)(arrayOfFunctionAddresses[funcOrdinal] + (PCHAR)hNtdll);

        if (funcName[0] == 'N' && funcName[1] == 't')
        {
            DWORD number = *((PDWORD)(funcAddress + 1));
            if (*(PBYTE)funcAddress != MOV_OPCODE)  continue;
            if (number > pExportDirectory->NumberOfNames)  continue;    
            ++ dwCount;
        }
    }
    *NumOfFunName = dwCount;

    pSsdtName = (PSSDT_NAME)GlobalAlloc(GPTR, dwCount * sizeof(SSDT_NAME));
    if (pSsdtName == NULL)  return NULL;

    for (UINT i = 0; i < pExportDirectory->NumberOfNames; i++)
    {
        char* funcName = (char*)(arrayOfFunctionNames[i] + (PCHAR)hNtdll);
        DWORD funcOrdinal = arrayOfFunctionOrdinals[i] + pExportDirectory->Base - 1;
        DWORD funcAddress = (DWORD)(arrayOfFunctionAddresses[funcOrdinal] + (PCHAR)hNtdll);

        if (funcName[0] == 'N' && funcName[1] == 't')
        {
            DWORD number = *((PDWORD)(funcAddress + 1));
            if (*(PBYTE)funcAddress != MOV_OPCODE)  continue;
            if (number > pExportDirectory->NumberOfNames)  continue;    
            strncpy_s(pSsdtName[number].FunName, 64, funcName, lstrlenA(funcName));
            pSsdtName[number].nIndex = number;
        }
    }
    return pSsdtName;
}


//////////////////////////////////////////////////////////////////////////////////
//
//	功能实现：解析内核文件(win32k.sys)，获取Shadow SSDT表中的服务函数原始地址
//	输入参数：*NumberOfAddresses返回原始Shaodw SSDT表中的服务函数个数
//	输出参数：返回DWORD类型的地址数组指针
//	注释：返回的缓冲区最终要用 GlobalFree 释放
//
///////////////////////////////////////////////////////////////////////////////////
PDWORD 
GetShadowSsdtNativeFunAddresses(PDWORD NumberOfAddresses)
{
    BOOL    bStatus = FALSE;
    char    KernelName[16] = { 0 };
    char    Win32KName[] = "Win32k.sys";
    DWORD   KernelBase, Win32kBase;
    HMODULE hKernelModule, hWin32KModule;
    WORD    wVersion;
    
    wVersion = GetCurrentOSVersion();
    
    if (wVersion > VER_W2K3SP2) {
        goto VersionHigh;
    }

    PDWORD  NativeAddresses = NULL;
    PDWORD  BufferData = NULL;
    DWORD   ImageBase = 0;
    PCHAR   BufferFile = NULL;
    WCHAR   Path[MAX_PATH] = { 0 };

    GetSystemDirectory(Path,MAX_PATH);
    wcscat_s(Path, L"\\");
    wcscat_s(Path, L"win32k.sys");

    //获取win32k.sys的.data节数据
    BufferData = (PDWORD)GetSectionPoint(Path, ".data", &ImageBase, &BufferFile);
    switch (wVersion)
    {
    case VER_WXPSP2:
    case VER_WXPSP3:
    case VER_W2K3SP2:
        {
            DWORD	Number = 0;
            UINT    i;

            for (i = 0; BufferData[i] != 0x00000101; i++);
            if(NumberOfAddresses) {
                *NumberOfAddresses = i;
            }

            Number = i;

            NativeAddresses = (PDWORD)GlobalAlloc(GPTR, Number * sizeof(DWORD));
            if(NativeAddresses) {
                for(i = 0; i < Number; i++) {
                    NativeAddresses[i] = BufferData[i];
                }
            }
            return NativeAddresses;
        }
    case VER_WXPSP1:
        {
            DWORD	Number = 0;
            DWORD	i = 0;

            BufferData=(PDWORD)((DWORD)BufferData + 0x6540);

            for(i = 0; BufferData[i] != 0x00000101; i++);
            if(NumberOfAddresses) {
                *NumberOfAddresses = i;
            }

            Number = i;

            NativeAddresses=(PDWORD)GlobalAlloc(GPTR, Number * sizeof(DWORD));
            if(NativeAddresses) {
                for(i = 0; i < Number; i++) {
                    NativeAddresses[i] = BufferData[i];
                }
            }
            return NativeAddresses;
        }
    case VER_W2K3:
        {
            DWORD	Number = 0;
            DWORD	i = 0;

            BufferData = (PDWORD)((DWORD)BufferData + 0x9524);

            for (i = 0; BufferData[i] != 0x00000101; i++) ;
            if(NumberOfAddresses) {
                *NumberOfAddresses = i;
            }

            Number = i;

            NativeAddresses=(PDWORD)GlobalAlloc(GPTR, Number * sizeof(DWORD));
            if(NativeAddresses) {
                for(i = 0; i < Number; i++) {
                    NativeAddresses[i] = BufferData[i];
                }
            }
            return NativeAddresses;
        }
    }
    GlobalFree(BufferFile);

VersionHigh:

    bStatus = GetKernelInformation(KernelName, 16, &KernelBase);
    if (bStatus == FALSE)   return NULL;
    bStatus = GetKernelModuleBaseByName(Win32KName, &Win32kBase);
    if (bStatus == FALSE)   return NULL;

    hKernelModule = LoadLibraryExA(KernelName, 0, DONT_RESOLVE_DLL_REFERENCES);
    hWin32KModule = LoadLibraryExA(Win32KName, 0, DONT_RESOLVE_DLL_REFERENCES);

    //////////////////////////////////////////////////////////////////////////

    PIMAGE_OPTIONAL_HEADER pWin32KOptionalHeader;
    PIMAGE_IMPORT_DESCRIPTOR pImportTable;
    char* DLLName;
    DWORD dwKeAddSystemServiceTable;

    pWin32KOptionalHeader = (PIMAGE_OPTIONAL_HEADER)
        ((DWORD)hWin32KModule + ((PIMAGE_DOS_HEADER)hWin32KModule)->e_lfanew + 0x18);
    pImportTable = (PIMAGE_IMPORT_DESCRIPTOR)
        ((DWORD)hWin32KModule + pWin32KOptionalHeader->DataDirectory[1].VirtualAddress);

    while (pImportTable->FirstThunk || pImportTable->Name || pImportTable->OriginalFirstThunk)
    {
        DLLName = (char*)(pImportTable->Name + (DWORD)hWin32KModule);

        if (_stricmp(DLLName, "NTOSKRNL.EXE") == 0)  break;

        pImportTable ++;
    }

    
    PIMAGE_THUNK_DATA OrigFirstThunk = (PIMAGE_THUNK_DATA)
        ((DWORD)hWin32KModule + pImportTable->OriginalFirstThunk);
    PIMAGE_THUNK_DATA FirstThunk = (PIMAGE_THUNK_DATA)
        ((DWORD)hWin32KModule + pImportTable->FirstThunk);

    for(DWORD index = 0; &(OrigFirstThunk[index]); index++) {
        if(!(OrigFirstThunk[index].u1.Ordinal & IMAGE_ORDINAL_FLAG32 )) {
            PIMAGE_IMPORT_BY_NAME pFunName = (PIMAGE_IMPORT_BY_NAME)
                (OrigFirstThunk[index].u1.ForwarderString + (DWORD)hWin32KModule);  
            if(_stricmp((char*)pFunName->Name, "KeAddSystemServiceTable") == 0)  {
                dwKeAddSystemServiceTable = FirstThunk[index].u1.Function;
                break;
            }
        }
    }

/*++

    win2k sp4~winxp sp1首先定位加载的win32k.sys的入口点(静态文件的麻烦一些),
    然后搜索FF 15 DD CC BB AA特征,如果0xAABBCCDD指向的内容是KeAddSystemServiceTable
    则当前指令的前面4字节就是表的实际地址

    bf9ae613 68109099bf      push    offset win32k!W32pArgumentTable (bf999010)
    bf9ae618 ff350c9099bf    push    dword ptr [win32k!W32pServiceLimit (bf99900c)]
    bf9ae61e 893520359abf    mov     dword ptr [win32k!countTable (bf9a3520)],esi
    bf9ae624 56              push    esi
    bf9ae625 68008399bf      push    offset win32k!W32pServiceTable (bf998300)
    bf9ae62a ff15d8b498bf    call    dword ptr [win32k!_imp__KeAddSystemServiceTable (bf98b4d8)]
    bf9ae630 e8fd0a0000      call    win32k!InitCreateUserCrit (bf9af132)

    得到的地址减去win32k.sys的实际加载地址并转换为RAW OFFSET就是shadow table在文件中的偏移

--*/
    DWORD ShadowSsdtTable = 0;
    DWORD dwEntryPoint = pWin32KOptionalHeader->AddressOfEntryPoint + (DWORD)hWin32KModule;
    for (DWORD CurrentAddress = dwEntryPoint; 
         CurrentAddress < dwEntryPoint + 0x1000; 
         CurrentAddress ++ ) 
    {
        if(*(PWORD)CurrentAddress == 0x15FF) {
            DWORD FuncAddress = *(PDWORD)(*(PULONG)(CurrentAddress + 2) - 
                        pWin32KOptionalHeader->ImageBase + (DWORD)hWin32KModule);
            if (dwKeAddSystemServiceTable == FuncAddress) {
                ShadowSsdtTable = *(PDWORD)(CurrentAddress - 4) - 
                        pWin32KOptionalHeader->ImageBase + (DWORD)hWin32KModule;
                break;
            }
        }   
    }
    
    PDWORD Buffer = 0;
    DWORD Number = 0;

    switch (wVersion)
    {
    case VER_VISTA11:
    case VER_VISTASP1:
    case VER_VISTAULT:
        {
            Number = VISTA_FUNCTION_NUMBER;
            break;
        }
    case VER_WINDOWS7:
        {
            Number = WIN7_FUNCTION_NUMBER;
            break;
        }
    default:
        {
            Number = 0;
            break;
        }	
    }

    Buffer = (PDWORD)GlobalAlloc(GPTR, (Number + 1) * sizeof(DWORD));
    if (!Buffer)  { *NumberOfAddresses = 0; return NULL; }

    for (ULONG ServiceId = 0; ServiceId < Number; ServiceId ++)
    {
        Buffer[ServiceId] = *(PDWORD)(ShadowSsdtTable + ServiceId * 4) - 
                                pWin32KOptionalHeader->ImageBase + Win32kBase;
    }
    *NumberOfAddresses = Number;
    return Buffer;
}

//////////////////////////////////////////////////////////////////////////