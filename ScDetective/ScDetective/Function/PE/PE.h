
#ifndef _PE_H_
#define _PE_H_

#define ibaseDD *(PDWORD)&ibase

DWORD 
GetHeaders(
    PCHAR ibase,
    PIMAGE_FILE_HEADER *pfh,
    PIMAGE_OPTIONAL_HEADER *poh,
    PIMAGE_SECTION_HEADER *psh
    );

PCHAR
GetSectionPoint(
    PCTSTR  ImagePath,
    PCSTR   SectionName,
    PDWORD  ImageBase,
    char**  pFile
    );


BOOL 
GetFuncAddressFromIAT(
    HMODULE ModuleBase,
    PSTR    pszFuncName,
    PULONG  pFuncAddress
    );


#endif

