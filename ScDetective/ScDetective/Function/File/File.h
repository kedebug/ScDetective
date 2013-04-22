

#ifndef __FILE_H__
#define __FILE_H__

typedef struct _LANGUAGE_CODE_PAGE {   
    WORD   wLanguage;   
    WORD   wCodePage;   
} LANGUAGE_CODE_PAGE, * PLANGUAGE_CODE_PAGE; 

BOOL 
GetFileCorporation(
    PWCHAR pszFileName, 
    PWCHAR pszFileCorporation
    );
VOID 
BrowseFolder(
    LPCTSTR szImagePath
    );
VOID ModifyFileImagePath( 
    PSTR    pszFilePath, 
    PSTR    pszWin32Name,
    ULONG   cbszWin32Name
    );
VOID
GetFileNameByImagePath(
    LPTSTR  pszImagePath,
    LPTSTR  pszFileName
    );

#endif

