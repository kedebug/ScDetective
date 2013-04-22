
#ifndef _FILE_H_
#define _FILE_H_

typedef struct _FILE_LIST_HEAD {
    LIST_ENTRY  FileListHead;
    ULONG       NumberOfItems;
} FILE_LIST_HEAD, * PFILE_LIST_HEAD;

//////////////////////////////////////////////////////////////////////////

PFILE_LIST_HEAD g_FileListHead = NULL;

//////////////////////////////////////////////////////////////////////////

NTSTATUS
ScfsQueryFileNameString(
    PFILE_OBJECT FileObject, 
    PUNICODE_STRING NameString
    );

PUNICODE_STRING
Convert2KernelLinkName(
    PUNICODE_STRING DosLinkName
    );

PFILE_LIST_HEAD
ScfsQueryDirectoryInformation(
    PWCHAR  pszDirectory
    );

ULONG
ExCopyFileList2Buffer(
    PFILE_INFO  FileInfo
    );

#endif

