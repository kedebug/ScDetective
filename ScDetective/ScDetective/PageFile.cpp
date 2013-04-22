// PageFile.cpp : 实现文件
//

#include "stdafx.h"
#include "ScDetective.h"
#include "PageFile.h"

extern CDriver theDriver;
extern CStatic* theStatus;
// CPageFile 对话框

IMPLEMENT_DYNAMIC(CPageFile, CDialog)

CPageFile::CPageFile(CWnd* pParent /*=NULL*/)
	: CDialog(CPageFile::IDD, pParent)
{

}

CPageFile::~CPageFile()
{
}

void CPageFile::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TREE1, m_TreeFile);
    DDX_Control(pDX, IDC_LIST6, m_ListFile);
}


BEGIN_MESSAGE_MAP(CPageFile, CDialog)
    ON_NOTIFY(TVN_SELCHANGED, IDC_TREE1, &CPageFile::OnTvnSelchangedTreeFile)
    ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()


// CPageFile 消息处理程序

BOOL CPageFile::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_ListFile.SetExtendedStyle(m_ListFile.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_ListFile.InsertColumn( 0, L"文件名",   LVCFMT_LEFT, 150, -1);
    m_ListFile.InsertColumn( 1, L"大小",     LVCFMT_LEFT, 80, -1);
    m_ListFile.InsertColumn( 2, L"占用空间", LVCFMT_LEFT, 80, -1);
    m_ListFile.InsertColumn( 3, L"创建时间", LVCFMT_LEFT, 80, -1);
    m_ListFile.InsertColumn( 4, L"修改时间", LVCFMT_LEFT, 80, -1);  
    m_ListFile.InsertColumn( 5, L"常规属性", LVCFMT_CENTER, 80, -1);

    m_ImageList.Create(18, 18, ILC_COLOR32, 10, 1024);
    m_ImageList.Add(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_MYPC)));
    m_ImageList.Add(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_REMOVEABLE)));
    m_ImageList.Add(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_FIXED)));
    m_ImageList.Add(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_REMOTE)));
    m_ImageList.Add(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_CDROM)));
    m_ImageList.Add(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_RAMDISK)));
    m_ImageList.Add(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_FLODER)));
    m_TreeFile.SetImageList(&m_ImageList, TVSIL_NORMAL);

    WCHAR Buffer[260];
    PWCHAR szDrives = Buffer;
    ULONG IconIndex;
    CString NameString;

    m_hRoot = m_TreeFile.InsertItem(L"我的电脑", 0, 0);
    GetLogicalDriveStrings(260, Buffer);

    for (; szDrives[0]; szDrives = szDrives + wcslen(szDrives) + 1)
    {
        switch (GetDriveType(szDrives))
        {
        case DRIVE_REMOVABLE:
            NameString = L"可移动磁盘(";
            NameString += szDrives;
            NameString.Delete(NameString.GetLength() - 1, 1);
            NameString += L")";
            IconIndex = 1;
            break;
        case DRIVE_FIXED:
            NameString = L"本地磁盘(";
            NameString += szDrives;
            NameString.Delete(NameString.GetLength() - 1, 1);
            NameString += L")";
            IconIndex = 2;
            break;
        case DRIVE_REMOTE:
            NameString = L"网络磁盘(";
            NameString += szDrives;
            NameString.Delete(NameString.GetLength() - 1, 1);
            NameString += L")";
            IconIndex = 3;
            break;
        case DRIVE_CDROM:
            NameString = L"光驱(";
            NameString += szDrives;
            NameString.Delete(NameString.GetLength() - 1, 1);
            NameString += L")";
            IconIndex = 4;
            break;
        case DRIVE_RAMDISK:
            IconIndex = 5;
        default:
            IconIndex = 2;
        }
        m_TreeFile.InsertItem(NameString, IconIndex, IconIndex, m_hRoot);
    }
    m_TreeFile.Expand(m_hRoot, TVE_EXPAND); 

    return TRUE; 
}

void CPageFile::OnTvnSelchangedTreeFile(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
    
    HTREEITEM hItem = pNMTreeView->itemNew.hItem;

    if (hItem != m_hRoot) {
        ListDirectory(hItem);
    }
    
    pResult[0] = 0;
}

BOOL CPageFile::JudgeSubDirectory(PWCHAR pszPath)
{
    return FALSE;
}

void CPageFile::ListDirectory(HTREEITEM hItem)
{
    DeleteChildren(hItem);
    m_ListFile.DeleteAllItems();

    if (theDriver.m_hDriver == NULL || 
        theDriver.m_hDriver == INVALID_HANDLE_VALUE)
    {
        return;
    }

    WCHAR szDriectory[261] = {0};
    GetItemPath(hItem, szDriectory, 260);

    DWORD BytesReturned;
    ULONG NeedLen = 0;
    PFILE_INFO FileInformation = NULL;
    PVOID AllocBuffer;
    ULONG NumberOfItems;

    BytesReturned = theDriver.IoControl(IOCTL_LIST_DIRECTORY, 
                                        szDriectory, 260 * 2, 
                                        &NeedLen, sizeof(ULONG));

    if (NeedLen == 0)  return;
    AllocBuffer = GlobalAlloc(GPTR, NeedLen);
    FileInformation = (PFILE_INFO)AllocBuffer;

    if (FileInformation != NULL)
    {
        BytesReturned = theDriver.IoControl(IOCTL_LIST_DIRECTORY, 
                                            szDriectory, 260 * 2, 
                                            FileInformation, NeedLen);

        NumberOfItems = NeedLen / sizeof(FILE_INFO);
    }
    else return ;

    WCHAR szFileSize[64];
    WCHAR szAllocationSize[64];
    WCHAR szCreationTime[128];
    WCHAR szLastWriteTime[128];
    WCHAR szAttributes[16];

    for (ULONG i = 0; i < NumberOfItems; i++, FileInformation++)
    {
        memset(szFileSize,      0, 2 * 64);
        memset(szAllocationSize,0, 2 * 64);
        memset(szCreationTime,  0, 2 * 128);
        memset(szLastWriteTime, 0, 2 * 128);
        memset(szAttributes,    0, 2 * 16);

        if (FileInformation->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            m_TreeFile.InsertItem(FileInformation->FileName, 6, 6, hItem);
        } else {
            wsprintf(szAllocationSize, L"%I64d", FileInformation->AllocationSize);
            wsprintf(szFileSize, L"%I64d", FileInformation->EndOfFile);
            wsprintf(szCreationTime, L"%d-%02d-%02d %02d:%02d:%02d", 
                FileInformation->CreationTime.Year, FileInformation->CreationTime.Month, 
                FileInformation->CreationTime.Day, FileInformation->CreationTime.Hour,
                FileInformation->CreationTime.Minute, FileInformation->CreationTime.Second);
            wsprintf(szLastWriteTime, L"%d-%02d-%02d %02d:%02d:%02d", 
                FileInformation->LastWriteTime.Year, FileInformation->LastWriteTime.Month, 
                FileInformation->LastWriteTime.Day, FileInformation->LastWriteTime.Hour,
                FileInformation->LastWriteTime.Minute, FileInformation->LastWriteTime.Second);
            wcscpy_s(szAttributes, 16, L"-"); 

            int nItemNum = m_ListFile.GetItemCount();
            m_ListFile.InsertItem(nItemNum, FileInformation->FileName);
            m_ListFile.SetItemText(nItemNum, 1, szFileSize);
            m_ListFile.SetItemText(nItemNum, 2, szAllocationSize);
            m_ListFile.SetItemText(nItemNum, 3, szCreationTime);
            m_ListFile.SetItemText(nItemNum, 4, szLastWriteTime);
            m_ListFile.SetItemText(nItemNum, 5, szAttributes);
        }
    }
    GlobalFree(AllocBuffer);
}

void CPageFile::DeleteChildren(HTREEITEM hItem)
{
    HTREEITEM hSubItem, hNextItem;

    hSubItem = m_TreeFile.GetChildItem(hItem);
    if (hSubItem == 0)  return;

    while (hNextItem = m_TreeFile.GetNextSiblingItem(hSubItem))
    {
        m_TreeFile.DeleteItem(hSubItem);
        hSubItem = hNextItem;
    }

    m_TreeFile.DeleteItem(hSubItem);
}

void CPageFile::GetItemPath(HTREEITEM hItem, PWCHAR pszPath, ULONG cbszPath)
{
    CString szPath;
    CString szBuffer;
    HTREEITEM hParent;
    
    if (m_TreeFile.GetParentItem(hItem) != m_hRoot) {
        szPath = m_TreeFile.GetItemText(hItem);
    } else {
        szPath = m_TreeFile.GetItemText(hItem);
        WCHAR ch = szPath.GetAt(szPath.Find(L":") - 1);
        szPath = ch;
        szPath += L":\\";
        wcscpy_s(pszPath, cbszPath, szPath.GetBuffer(0));
        return ;
    }
    
    while (hParent = m_TreeFile.GetParentItem(hItem))
    {
        if (m_TreeFile.GetParentItem(hParent) != m_hRoot)
        {
            szBuffer = m_TreeFile.GetItemText(hParent);

            if (szBuffer.Find(L"\\") == -1) 
            {
                szBuffer += L"\\";
            }

            szPath = szBuffer + szPath;
            hItem = hParent;

        } 
        else  
        {
            szBuffer = m_TreeFile.GetItemText(hParent);
            WCHAR ch = szBuffer.GetAt(szBuffer.Find(L":") - 1);
            szBuffer = ch;
            szBuffer += L":\\";
            szPath = szBuffer + szPath;
            break;
        }
    }
    wcscpy_s(pszPath, cbszPath, szPath.GetBuffer(0));
}

void CPageFile::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CDialog::OnShowWindow(bShow, nStatus);

    theStatus->SetWindowText(L" ");
}
