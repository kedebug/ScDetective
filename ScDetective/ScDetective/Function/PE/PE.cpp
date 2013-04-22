
#include "stdafx.h"


///////////////////////////////////////////////////////////////////////////////////
//
//	功能实现：解析PE文件，获取PE结构中各个头的指针
//	输入参数：ibase为映像基地址；
//			  pfh,poh,psh分别为IMAGE_FILE_HEADER，IMAGE_OPTIONAL_HEADER，
//			  IMAGE_SECTION_HEADER的指针
//	输出参数：返回TRUE表示获取成功，否则失败
//
///////////////////////////////////////////////////////////////////////////////////
DWORD GetHeaders(
    PCHAR ibase,
    PIMAGE_FILE_HEADER *pfh,
    PIMAGE_OPTIONAL_HEADER *poh,
    PIMAGE_SECTION_HEADER *psh
    )

{
    TRACE0("获取PE文件头信息");
    PIMAGE_DOS_HEADER mzhead=(PIMAGE_DOS_HEADER)ibase;

    if    ((mzhead->e_magic!=IMAGE_DOS_SIGNATURE) ||        
        (ibaseDD[mzhead->e_lfanew]!=IMAGE_NT_SIGNATURE))
        return FALSE;

    *pfh=(PIMAGE_FILE_HEADER)&ibase[mzhead->e_lfanew];
    if (((PIMAGE_NT_HEADERS)*pfh)->Signature!=IMAGE_NT_SIGNATURE) 
        return FALSE;
    *pfh=(PIMAGE_FILE_HEADER)((PBYTE)*pfh+sizeof(IMAGE_NT_SIGNATURE));

    *poh=(PIMAGE_OPTIONAL_HEADER)((PBYTE)*pfh+sizeof(IMAGE_FILE_HEADER));
    if ((*poh)->Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        return FALSE;

    *psh=(PIMAGE_SECTION_HEADER)((PBYTE)*poh+sizeof(IMAGE_OPTIONAL_HEADER));
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////
//
//	功能实现：获取以加载模块中函数地址（RVA）
//	输入参数：ModuleBase为模块加载地址，
//			 pszFuncName为函数名称
//           pFuncAddress 为输出函数地址（RVA）
//	输出参数：是否查找成功
//
///////////////////////////////////////////////////////////////////////////////////
BOOL 
GetFuncAddressFromIAT(HMODULE ModuleBase, PSTR pszFuncName, PULONG pFuncAddress)
{
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////
//
//	功能实现：获取指定文件中的指定节的指针
//	输入参数：ImagePath为要获取指定节指针的映像路径
//			  SetionName为要查找的节名
//			  ImageBase为返回的映像的默认加载基地址;
//			  pFile为存放内存映像的缓冲区地址;
//	输出参数：返回指向指定节数据的指针
//
///////////////////////////////////////////////////////////////////////////////////
PCHAR 
GetSectionPoint(PCTSTR ImagePath, PCSTR SectionName, PDWORD ImageBase, char** pFile)
{
    HANDLE  hFile;
    DWORD   FileSize = 0;
    DWORD   ReadSize = 0;
    char*   BufferFile = NULL;

    hFile = CreateFile(ImagePath, GENERIC_READ, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)  return NULL;

    FileSize = GetFileSize(hFile, 0);
    
    BufferFile = (char*)GlobalAlloc(GPTR, FileSize);
    if (BufferFile == NULL) {
        CloseHandle(hFile);
        return NULL;
    }
    
    ReadFile(hFile, BufferFile, FileSize, &ReadSize, NULL);
    if (ReadSize <= 0) {
        GlobalFree(BufferFile);
        CloseHandle(hFile);
        return NULL;
    }
    CloseHandle(hFile);

    PIMAGE_FILE_HEADER      FileHeader;
    PIMAGE_OPTIONAL_HEADER  OptionalHeader;
    PIMAGE_SECTION_HEADER   SectionHeader;

    GetHeaders(BufferFile, &FileHeader, &OptionalHeader, &SectionHeader);  
    if (ImageBase != NULL)  *ImageBase = OptionalHeader->ImageBase;

    for (int i = 0; i < FileHeader->NumberOfSections; i ++, SectionHeader ++)  
    {
        if (_stricmp((char*)SectionHeader->Name, SectionName) == 0) 
        {
            if (SectionHeader->PointerToRawData == NULL) 
            {
                GlobalFree(BufferFile);
                return NULL;
            }
            break;
        }
    }

    *pFile = BufferFile;
    char* Buffer = (char*)(BufferFile + SectionHeader->PointerToRawData);
    return Buffer;
}