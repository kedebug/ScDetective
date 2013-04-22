/*
 *	定义一些常用的文件数据结构
 */
/////////////////////////////////////////////////////////////////////
// Undocumented structures missing in ntddk.h
/////////////////////////////////////////////////////////////////////
typedef struct _FILE_INTERNAL_INFORMATION { // Information Class 6
  LARGE_INTEGER FileId;
} FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;

typedef struct _FILE_EA_INFORMATION { // Information Class 7
  ULONG EaInformationLength;
} FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;

typedef struct _FILE_ACCESS_INFORMATION { // Information Class 8
  ACCESS_MASK GrantedAccess;
} FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;

typedef struct _FILE_MODE_INFORMATION { // Information Class 16
  ULONG Mode;
} FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;

typedef struct _FILE_ALLOCATION_INFORMATION { // Information Class 19
  LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFORMATION, *PFILE_ALLOCATION_INFORMATION;

typedef struct _FILE_ALL_INFORMATION { // Information Class 18
  FILE_BASIC_INFORMATION BasicInformation;
  FILE_STANDARD_INFORMATION StandardInformation;
  FILE_INTERNAL_INFORMATION InternalInformation;
  FILE_EA_INFORMATION EaInformation;
  FILE_ACCESS_INFORMATION AccessInformation;
  FILE_POSITION_INFORMATION PositionInformation;
  FILE_MODE_INFORMATION ModeInformation;
  FILE_ALIGNMENT_INFORMATION AlignmentInformation;
  FILE_NAME_INFORMATION NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;

typedef struct tag_QUERY_DIRECTORY
{
  ULONG Length;
  PUNICODE_STRING FileName;
  FILE_INFORMATION_CLASS FileInformationClass;
  ULONG FileIndex;
} QUERY_DIRECTORY, *PQUERY_DIRECTORY;

typedef struct tag_FQD_SmallCommonBlock
{
  ULONG NextEntryOffset;
  ULONG FileIndex;
} FQD_SmallCommonBlock, *PFQD_SmallCommonBlock;

typedef struct tag_FQD_FILE_ATTR
{
  TIME CreationTime;
  TIME LastAccessTime;
  TIME LastWriteTime;
  TIME ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
} FQD_FILE_ATTR, *PFQD_FILE_ATTR;

typedef struct tag_FQD_CommonBlock
{
  FQD_SmallCommonBlock SmallCommonBlock;
  FQD_FILE_ATTR FileAttr;
  ULONG FileNameLength;
} FQD_CommonBlock, *PFQD_CommonBlock;

typedef struct _KFILE_DIRECTORY_INFORMATION
{
  FQD_CommonBlock CommonBlock;
  
  WCHAR FileName[ANYSIZE_ARRAY];
} KFILE_DIRECTORY_INFORMATION, *PKFILE_DIRECTORY_INFORMATION;

typedef struct _KFILE_FULL_DIR_INFORMATION
{
  FQD_CommonBlock CommonBlock;
  
  ULONG EaSize;
  WCHAR FileName[ANYSIZE_ARRAY];
} KFILE_FULL_DIR_INFORMATION, *PKFILE_FULL_DIR_INFORMATION;

typedef struct _KFILE_BOTH_DIR_INFORMATION
{
  FQD_CommonBlock CommonBlock;
  
  ULONG EaSize;
  USHORT ShortFileNameLength;
  WCHAR ShortFileName[12];
  WCHAR FileName[ANYSIZE_ARRAY];
} KFILE_BOTH_DIR_INFORMATION, *PKFILE_BOTH_DIR_INFORMATION;
/////////////////////////////////////////////////////////////////////
// Undocumented structures missing in ntddk.h
/////////////////////////////////////////////////////////////////////
typedef struct _FILE_DIRECTORY_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef struct _FILE_FULL_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    WCHAR FileName[1];
} FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;

typedef struct _FILE_ID_FULL_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    LARGE_INTEGER FileId;
    WCHAR FileName[1];
} FILE_ID_FULL_DIR_INFORMATION, *PFILE_ID_FULL_DIR_INFORMATION;

typedef struct _FILE_BOTH_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    WCHAR FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

typedef struct _FILE_ID_BOTH_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    LARGE_INTEGER FileId;
    WCHAR FileName[1];
} FILE_ID_BOTH_DIR_INFORMATION, *PFILE_ID_BOTH_DIR_INFORMATION;

typedef struct _FILE_NAMES_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

//申明未存档的API函数
NTSYSAPI NTSTATUS
ObQueryNameString(
				  IN  PVOID Object,
				  OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
				  IN  ULONG Length,
				  OUT PULONG ReturnLength
				  );

NTSYSAPI NTSTATUS
ObReferenceObjectByName(
						IN PUNICODE_STRING ObjectPath,
						IN ULONG Attributes,
						IN PACCESS_STATE PassedAccessState OPTIONAL,
						IN ACCESS_MASK DesiredAccess OPTIONAL,
						IN POBJECT_TYPE ObjectType,
						IN KPROCESSOR_MODE AccessMode,
						IN OUT PVOID ParseContext OPTIONAL,
						OUT PVOID *ObjectPtr);