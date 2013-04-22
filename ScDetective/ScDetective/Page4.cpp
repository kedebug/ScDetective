// Page4.cpp : 实现文件
//

#include "stdafx.h"

extern CDriver theDriver;
extern CStatic* theStatus;

ULONG g_EProcess;
WCHAR g_szImageName[64];

// CPage4 对话框

IMPLEMENT_DYNAMIC(CPage4, CDialog)

CPage4::CPage4(CWnd* pParent /*=NULL*/)
	: CDialog(CPage4::IDD, pParent)
{
    memset(StatusBuffer, 0, 128 * sizeof(WCHAR));
}

CPage4::~CPage4()
{
}

void CPage4::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST4, m_ListProcess);
}


BEGIN_MESSAGE_MAP(CPage4, CDialog)
    ON_WM_SHOWWINDOW()
    ON_COMMAND(ID_MENU_32781, &CPage4::OnMenuRefreshProcess)
    ON_NOTIFY(NM_RCLICK, IDC_LIST4, &CPage4::OnNMRClickListProcess)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST4, &CPage4::OnNMCustomdrawListProcess)
    ON_COMMAND(ID_MENU_32783, &CPage4::OnMenuShowProcessModules)
    ON_COMMAND(ID_MENU_32784, &CPage4::OnMenuShowProcessThreads)
    ON_COMMAND(ID_MENU_32785, &CPage4::OnMenuShowProcessHandles)
    ON_COMMAND(ID_MENU_32782, &CPage4::OnMenuLocate2File)
END_MESSAGE_MAP()


// CPage4 消息处理程序

BOOL CPage4::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_ListProcess.SetExtendedStyle(m_ListProcess.GetExtendedStyle() | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_ListProcess.InsertColumn( 0, L"映像名称",  LVCFMT_CENTER, 85, -1);
    m_ListProcess.InsertColumn( 1, L"进程Id",    LVCFMT_RIGHT, 50, -1);
    m_ListProcess.InsertColumn( 2, L"父进程Id",  LVCFMT_RIGHT, 60, -1);
    m_ListProcess.InsertColumn( 3, L"隐藏",      LVCFMT_CENTER, 40, -1);
    m_ListProcess.InsertColumn( 4, L"映像路径",  LVCFMT_LEFT, 235, -1);
    m_ListProcess.InsertColumn( 5, L"EPROCESS",  LVCFMT_CENTER, 80, -1);  
    m_ListProcess.InsertColumn( 6, L"应用层访问", LVCFMT_CENTER, 77, -1);
    m_ListProcess.InsertColumn( 7, L"文件厂商",   LVCFMT_LEFT, 140, -1);

    OnMenuRefreshProcess();

    return TRUE;  
}

void CPage4::OnMenuRefreshProcess()
{
    m_ListProcess.DeleteAllItems();
    if (theDriver.m_hDriver == NULL || 
        theDriver.m_hDriver == INVALID_HANDLE_VALUE)
    {
        return;
    }

    DWORD BytesReturned;
    ULONG NeedLen;
    PPROCESS_INFO ProcessInformation = NULL;
    PVOID AllocBuffer;
    ULONG NumberOfProcess;
    ULONG NumberOfUnAccess = 0;
    ULONG NumberOfHidden = 0; 

    BytesReturned = theDriver.IoControl(IOCTL_GET_PROCESSES, 
                                        NULL, 0, 
                                        &NeedLen, 
                                        sizeof(ULONG));
    AllocBuffer = GlobalAlloc(GPTR, NeedLen);
    ProcessInformation = (PPROCESS_INFO)AllocBuffer;

    if (ProcessInformation != NULL)
    {
        BytesReturned = theDriver.IoControl(IOCTL_GET_PROCESSES, 
                                            NULL, 0, 
                                            ProcessInformation, 
                                            NeedLen);
        NumberOfProcess = NeedLen / sizeof(PROCESS_INFO);
    }
    else return ;

    HANDLE  hProcess;
    WCHAR   szImageName[64];
    WCHAR   szFullImagePath[MAX_PATH+2];
    WCHAR   szProcessId[8];
    WCHAR   szHidden[8];
    WCHAR   szInheritedPid[8];
    WCHAR   szEProcess[16];
    WCHAR   szUserAccess[8];
    WCHAR   szFileInfomation[120];

    for (ULONG i = 0; i < NumberOfProcess; i++, ProcessInformation++)
    {
        memset(szImageName,         0, 64 * 2);
        memset(szFullImagePath,     0, 261 * 2);
        memset(szFileInfomation,    0, 120 * 2);
        memset(szProcessId,         0, 8 * 2);
        memset(szHidden,            0, 8 * 2);
        memset(szInheritedPid,      0, 8 * 2);
        memset(szEProcess,          0, 16 * 2);
        memset(szUserAccess,        0, 8 * 2);

        if (AquireUserAccess(ProcessInformation->UniqueProcessId, 
                             &hProcess)) {
            wsprintf(szUserAccess, L"-");
            //GetProcessImagePath(hProcess, szFullImagePath);
            CloseHandle(hProcess);
        } else {
            wsprintf(szUserAccess, L"拒绝");
            NumberOfUnAccess ++;
        }
        if (ProcessInformation->EProcess == 0)  continue;
        
        wcscpy_s(szFullImagePath, 260, ProcessInformation->ImagePath);
        GetFileNameByImagePath(szFullImagePath, szImageName);

        wsprintf(szProcessId, L"%d", ProcessInformation->UniqueProcessId);
        wsprintf(szInheritedPid, L"%d", ProcessInformation->InheritedProcessId);
        wsprintf(szEProcess, L"0x%08X", ProcessInformation->EProcess);

        if (ProcessInformation->UniqueProcessId != 0 ||
            ProcessInformation->UniqueProcessId != 4) {
            GetFileCorporation(szFullImagePath, szFileInfomation);
        }
        if (ProcessInformation->bHidden) {
            wcscpy_s(szHidden, 8, L"是");
            NumberOfHidden ++;
        } else {
            wcscpy_s(szHidden, 8, L"-");
        }
            
        int nItemNum = m_ListProcess.GetItemCount();
        m_ListProcess.InsertItem(nItemNum, szImageName);
        m_ListProcess.SetItemText(nItemNum, 1, szProcessId);
        m_ListProcess.SetItemText(nItemNum, 2, szInheritedPid);
        m_ListProcess.SetItemText(nItemNum, 3, szHidden);
        m_ListProcess.SetItemText(nItemNum, 4, ProcessInformation->ImagePath);
        m_ListProcess.SetItemText(nItemNum, 5, szEProcess);
        m_ListProcess.SetItemText(nItemNum, 6, szUserAccess);
        m_ListProcess.SetItemText(nItemNum, 7, szFileInfomation);
    }
    GlobalFree(AllocBuffer);

    wsprintf(StatusBuffer, L"当前进程数 :: %d - 应用层不可访问 :: %d - 隐藏进程数 :: %d", 
             NumberOfProcess, NumberOfUnAccess, NumberOfHidden);
    theStatus->SetWindowText(StatusBuffer);
}

void CPage4::OnNMRClickListProcess(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: 在此添加控件通知处理程序代码
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    if (pNMListView->iItem != -1 && pNMListView->iSubItem != -1)
    {
        CPoint pt;
        GetCursorPos(&pt);
        CMenu menu;
        menu.LoadMenu(IDR_MENU4_PROCESS);
        CMenu* pMenu = menu.GetSubMenu(0);
        pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, this);
    }
    *pResult = 0;
}

void CPage4::OnNMCustomdrawListProcess(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

    *pResult = CDRF_DODEFAULT;;

    if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
        *pResult = CDRF_NOTIFYITEMDRAW;
    else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
    else if ((CDDS_ITEMPREPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage)
    {
        COLORREF clrNewTextColor, clrNewBkColor;
        int nItem = static_cast<int>( pLVCD->nmcd.dwItemSpec);
        clrNewTextColor = RGB(0, 0, 0);

        WCHAR Buffer[64];
        m_ListProcess.GetItemText(nItem, 7, Buffer, 64);
        if (StrStr(Buffer, L"Microsoft ") == NULL)  clrNewTextColor = RGB(0, 0, 255);
        m_ListProcess.GetItemText(nItem, 3, Buffer, 64);
        if (StrStr(Buffer, L"是"))  clrNewTextColor = RGB(255, 0, 0);

        if( nItem % 2 == 0) clrNewBkColor = RGB(240, 240, 240);	  
        else  clrNewBkColor = RGB(255, 255, 255);	  

        pLVCD->clrText = clrNewTextColor;
        pLVCD->clrTextBk = clrNewBkColor;
        *pResult = CDRF_DODEFAULT;
    }
}

void CPage4::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CDialog::OnShowWindow(bShow, nStatus);

    theStatus->SetWindowText(StatusBuffer);
}
void CPage4::OnMenuShowProcessModules()
{
    WCHAR szEProcess[16] = { 0 };
    POSITION pos = m_ListProcess.GetFirstSelectedItemPosition();

    if (pos) 
    {
        int nItem = m_ListProcess.GetNextSelectedItem(pos);
        memset(g_szImageName, 0, 64 * 2);
        m_ListProcess.GetItemText(nItem, 0, g_szImageName, 64);
        m_ListProcess.GetItemText(nItem, 5, szEProcess, 16);
        swscanf_s(szEProcess + 2, L"%X", &g_EProcess);
    }
    m_ProcessModules.DoModal();
}

void CPage4::OnMenuShowProcessThreads()
{
    WCHAR szEProcess[16] = { 0 };
    POSITION pos = m_ListProcess.GetFirstSelectedItemPosition();

    if (pos) 
    {
        int nItem = m_ListProcess.GetNextSelectedItem(pos);
        memset(g_szImageName, 0, 64 * 2);
        m_ListProcess.GetItemText(nItem, 0, g_szImageName, 64);
        m_ListProcess.GetItemText(nItem, 5, szEProcess, 16);
        swscanf_s(szEProcess + 2, L"%X", &g_EProcess);
    }
    m_ProcessThreads.DoModal();
}

void CPage4::OnMenuShowProcessHandles()
{
    WCHAR szEProcess[16] = { 0 };
    POSITION pos = m_ListProcess.GetFirstSelectedItemPosition();

    if (pos) 
    {
        int nItem = m_ListProcess.GetNextSelectedItem(pos);
        memset(g_szImageName, 0, 64 * 2);
        m_ListProcess.GetItemText(nItem, 0, g_szImageName, 64);
        m_ListProcess.GetItemText(nItem, 5, szEProcess, 16);
        swscanf_s(szEProcess + 2, L"%X", &g_EProcess);
    }
    m_ProcessHandles.DoModal();
}

void CPage4::OnMenuLocate2File()
{
    WCHAR szImagePath[260] = { 0 };
    POSITION pos = m_ListProcess.GetFirstSelectedItemPosition();

    if (pos) 
    {
        int nItem = m_ListProcess.GetNextSelectedItem(pos);
        m_ListProcess.GetItemText(nItem, 3, szImagePath, 260);
        BrowseFolder(szImagePath);
    }
}

//////////////////////////////////////////////////////////////////////////

// CProcessThreads 对话框

IMPLEMENT_DYNAMIC(CProcessThreads, CDialog)

CProcessThreads::CProcessThreads(CWnd* pParent /*=NULL*/)
	: CDialog(CProcessThreads::IDD, pParent)
{

}

CProcessThreads::~CProcessThreads()
{
}

void CProcessThreads::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_PROCESS_THREADS, m_ListThreads);
}


BEGIN_MESSAGE_MAP(CProcessThreads, CDialog)
END_MESSAGE_MAP()


// CProcessThreads 消息处理程序

BOOL CProcessThreads::OnInitDialog()
{
    CDialog::OnInitDialog();
    
    m_ListThreads.SetExtendedStyle(m_ListThreads.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_ListThreads.InsertColumn( 0, L"线程Id",   LVCFMT_CENTER, 50, -1);
    m_ListThreads.InsertColumn( 1, L"ETHREAD",  LVCFMT_CENTER, 80, -1);
    m_ListThreads.InsertColumn( 2, L"Teb",      LVCFMT_CENTER, 80, -1);
    m_ListThreads.InsertColumn( 3, L"优先级",    LVCFMT_CENTER, 60, -1);
    m_ListThreads.InsertColumn( 4, L"线程入口",  LVCFMT_CENTER, 80, -1);  
    m_ListThreads.InsertColumn( 5, L"切换次数",  LVCFMT_RIGHT, 70, -1);
    m_ListThreads.InsertColumn( 6, L"线程状态",  LVCFMT_CENTER, 80, -1);

    ShowThreads(g_EProcess);

    return TRUE;  
}

void CProcessThreads::ShowThreads(ULONG EProcess)
{
    m_ListThreads.DeleteAllItems();
    if (theDriver.m_hDriver == NULL || 
        theDriver.m_hDriver == INVALID_HANDLE_VALUE)
    {
        return;
    }

    DWORD BytesReturned;
    ULONG NeedLen;
    PTHREAD_INFO ThreadInformation = NULL;
    PVOID AllocBuffer;
    ULONG NumberOfThread = 0;
    ULONG Number = 0;

    BytesReturned = theDriver.IoControl(IOCTL_GET_PROCESS_THREADS, 
                                        &EProcess, sizeof(ULONG), 
                                        &NeedLen, sizeof(ULONG));
    if (NeedLen == 0)  goto _End;

    AllocBuffer = GlobalAlloc(GPTR, NeedLen);
    ThreadInformation = (PTHREAD_INFO)AllocBuffer;

    if (ThreadInformation != NULL)
    {
        BytesReturned = theDriver.IoControl(IOCTL_GET_PROCESS_THREADS, 
                                            &EProcess, sizeof(ULONG), 
                                            ThreadInformation, NeedLen);
        NumberOfThread = NeedLen / sizeof(THREAD_INFO);
    }
    else return ;

    WCHAR   szThreadId[8];
    WCHAR   szEThread[16];
    WCHAR   szTeb[16];
    WCHAR   szPriority[8];
    WCHAR   szStartAddress[16];
    WCHAR   szContextSwitches[8];
    WCHAR   szState[8];

    for (ULONG i = 0; i < NumberOfThread; i++, ThreadInformation++)
    {
        memset(szThreadId,      0, 8 * 2);
        memset(szEThread,       0, 16 * 2);
        memset(szTeb,           0, 16 * 2);
        memset(szPriority,      0, 8 * 2);
        memset(szStartAddress,  0, 16 * 2);
        memset(szContextSwitches, 0, 8 * 2);
        memset(szState,         0, 8 * 2);
        
        wsprintf(szThreadId, L"%d", ThreadInformation->ThreadId);
        wsprintf(szEThread, L"0x%08X", ThreadInformation->EThread);
        wsprintf(szTeb, L"0x%08X", ThreadInformation->Teb);
        wsprintf(szPriority, L"%02d", ThreadInformation->Priority);
        wsprintf(szStartAddress, L"0x%08X", ThreadInformation->Win32StartAddress);
        wsprintf(szContextSwitches, L"%d", ThreadInformation->ContextSwitches);
        wsprintf(szState, L"%s", ThreadInformation->State);

        int nItemNum = m_ListThreads.GetItemCount();
        m_ListThreads.InsertItem(nItemNum, szThreadId);
        m_ListThreads.SetItemText(nItemNum, 1, szEThread);
        m_ListThreads.SetItemText(nItemNum, 2, szTeb);
        m_ListThreads.SetItemText(nItemNum, 3, szPriority);
        m_ListThreads.SetItemText(nItemNum, 4, szStartAddress);
        m_ListThreads.SetItemText(nItemNum, 5, szContextSwitches);
        m_ListThreads.SetItemText(nItemNum, 6, szState);
    }
_End:
    WCHAR szInfo[128];
    wsprintf(szInfo, L"  %s - 共找到线程 [ %d ]", g_szImageName, NumberOfThread);
    this->SetWindowText(szInfo);
    GlobalFree(AllocBuffer);
}


// CProcessModules 对话框

IMPLEMENT_DYNAMIC(CProcessModules, CDialog)

CProcessModules::CProcessModules(CWnd* pParent /*=NULL*/)
	: CDialog(CProcessModules::IDD, pParent)
{

}

CProcessModules::~CProcessModules()
{
}

void CProcessModules::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_PROCESS_MODULES, m_ListModules);
}


BEGIN_MESSAGE_MAP(CProcessModules, CDialog)
END_MESSAGE_MAP()


// CProcessModules 消息处理程序

BOOL CProcessModules::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_ListModules.SetExtendedStyle(m_ListModules.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_ListModules.InsertColumn( 0, L"模块路径",  LVCFMT_CENTER, 200, -1);
    m_ListModules.InsertColumn( 1, L"基地址",    LVCFMT_CENTER, 75, -1);
    m_ListModules.InsertColumn( 2, L"大小",      LVCFMT_CENTER, 75, -1);
    m_ListModules.InsertColumn( 3, L"文件厂商",   LVCFMT_LEFT, 145, -1);
  
    ShowModules(g_EProcess);
    
    return TRUE;  
}

void CProcessModules::ShowModules(ULONG EProcess)
{
    m_ListModules.DeleteAllItems();
    if (theDriver.m_hDriver == INVALID_HANDLE_VALUE || 
        theDriver.m_hDriver == NULL) {
        return;
    }

    DWORD BytesReturned;
    ULONG NeedLen;
    PMODULE_INFO ModuleInformation = NULL;
    PVOID AllocBuffer;
    ULONG NumberOfModule = 0;
    ULONG Number = 0;

    BytesReturned = theDriver.IoControl(IOCTL_GET_PROCESS_MODULES, 
                                        &EProcess, sizeof(ULONG), 
                                        &NeedLen, sizeof(ULONG));
    if (NeedLen == 0)  goto _End;

    AllocBuffer = GlobalAlloc(GPTR, NeedLen);
    ModuleInformation = (PMODULE_INFO)AllocBuffer;

    if (ModuleInformation != NULL) {
        BytesReturned = theDriver.IoControl(IOCTL_GET_PROCESS_MODULES, 
                                            &EProcess, sizeof(ULONG),
                                            ModuleInformation, NeedLen);
        NumberOfModule = NeedLen / sizeof(MODULE_INFO);
    }
    else return ;

    WCHAR   szDosName[MAX_PATH];
    WCHAR   szImageBase[16];
    WCHAR   szImageSize[16];
    WCHAR   szCorporation[128];

    for (ULONG i = 0; i < NumberOfModule; i++, ModuleInformation++)
    {   
        memset(szDosName, 0, MAX_PATH * 2);
        memset(szImageBase, 0, 16 * 2);
        memset(szImageSize, 0, 16 * 2);
        memset(szCorporation, 0, 128 * 2);

        wsprintf(szImageBase, L"0x%08X", ModuleInformation->BaseAddress);
        wsprintf(szImageSize, L"0x%08X", ModuleInformation->ImageSize);

        Convert2DosDeviceName(ModuleInformation->ImagePath, szDosName, 260);
        GetFileCorporation(szDosName, szCorporation);

        int nItemNum = m_ListModules.GetItemCount();
        m_ListModules.InsertItem(nItemNum, szDosName);
        m_ListModules.SetItemText(nItemNum, 1, szImageBase);
        m_ListModules.SetItemText(nItemNum, 2, szImageSize);
        m_ListModules.SetItemText(nItemNum, 3, szCorporation);
    }
_End:
    WCHAR szInfo[128];
    wsprintf(szInfo, L"  %s - 共找到模块 [ %d ]", g_szImageName, NumberOfModule);
    this->SetWindowText(szInfo);
    GlobalFree(AllocBuffer);
}

// CProcessHandles 对话框

IMPLEMENT_DYNAMIC(CProcessHandles, CDialog)

CProcessHandles::CProcessHandles(CWnd* pParent /*=NULL*/)
	: CDialog(CProcessHandles::IDD, pParent)
{

}

CProcessHandles::~CProcessHandles()
{
}

void CProcessHandles::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_PROCESS_HANDLES, m_ListHandles);
}


BEGIN_MESSAGE_MAP(CProcessHandles, CDialog)
END_MESSAGE_MAP()


// CProcessHandles 消息处理程序
