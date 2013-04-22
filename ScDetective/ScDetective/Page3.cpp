// Page3.cpp : 实现文件
//

#include "stdafx.h"

extern CDriver theDriver;
extern CStatic* theStatus;
WCHAR StatusBuffer[128];

// CPage3 对话框

IMPLEMENT_DYNAMIC(CPage3, CDialog)

CPage3::CPage3(CWnd* pParent /*=NULL*/)
	: CDialog(CPage3::IDD, pParent)
{

}

CPage3::~CPage3()
{
}

void CPage3::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TAB1, tableHook);
}


BEGIN_MESSAGE_MAP(CPage3, CDialog)
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CPage3::OnTcnSelchangeTabR3Hook)
    ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()


// CPage3 消息处理程序

BOOL CPage3::OnInitDialog()
{
    CDialog::OnInitDialog();

    PageArrayR3Hook[0] = &m_Page1;
    PageArrayR3Hook[1] = &m_Page2;

    tableHook.MoveWindow(CRect(-7,0,825,20), FALSE);

    tableHook.InsertItem(0, L"消息钩子");
    PageArrayR3Hook[0]->Create(IDD_MSGHOOK, this);

    tableHook.InsertItem(1, L"系统回调");
    PageArrayR3Hook[1]->Create(IDD_SYSROUTINE, this);

    tableHook.SetCurSel(0);

    RECT rect;
    this->GetClientRect(&rect);
    rect.top += 22;

    for (int i = 0; i < PAGE_R3_NUMBER; i++)
    {
        PageArrayR3Hook[i]->MoveWindow(&rect);
    }

    PageArrayR3Hook[0]->ShowWindow(SW_SHOWNA);

    return TRUE; 
}

// CSystemRoutine 消息处理程序

void CPage3::OnTcnSelchangeTabR3Hook(NMHDR *pNMHDR, LRESULT *pResult)
{
    int nCurSelect = tableHook.GetCurSel();
    ShowPageX(nCurSelect);
    *pResult = 0;
}

VOID CPage3::ShowPageX(int nIndex)
{
    for (int i = 0; i < PAGE_R3_NUMBER; i++)
    {
        PageArrayR3Hook[i]->ShowWindow(SW_HIDE);
    }
    PageArrayR3Hook[nIndex]->BringWindowToTop();
    PageArrayR3Hook[nIndex]->ShowWindow(SW_SHOWNORMAL);
    PageArrayR3Hook[nIndex]->UpdateWindow();
}

void CPage3::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CDialog::OnShowWindow(bShow, nStatus);

    theStatus->SetWindowText(StatusBuffer);
    if (bShow) {
        m_Page1.OnMenuRefreshMsgHook();
    }
    
}
// CMsgHook 对话框

IMPLEMENT_DYNAMIC(CMsgHook, CDialog)

CMsgHook::CMsgHook(CWnd* pParent /*=NULL*/)
	: CDialog(CMsgHook::IDD, pParent)
{
    m_pSharedInfo = 0;
}

CMsgHook::~CMsgHook()
{
}

void CMsgHook::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_MSGHOOK, m_ListMsgHook);
}


BEGIN_MESSAGE_MAP(CMsgHook, CDialog)
    ON_COMMAND(ID_MENU_32787, &CMsgHook::OnMenuRefreshMsgHook)
    ON_COMMAND(ID_MENU_32788, &CMsgHook::OnMenuUnhook)
    ON_COMMAND(ID_MENU_32789, &CMsgHook::OnMenuLocate2File)
    ON_NOTIFY(NM_RCLICK, IDC_LIST_MSGHOOK, &CMsgHook::OnNMRClickListMsghook)
END_MESSAGE_MAP()

BOOL CMsgHook::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_ListMsgHook.SetExtendedStyle(m_ListMsgHook.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_ListMsgHook.InsertColumn(0, L"句柄",      LVCFMT_CENTER,  80);
    m_ListMsgHook.InsertColumn(1, L"钩子类型",   LVCFMT_LEFT,    100);
    m_ListMsgHook.InsertColumn(2, L"钩子函数",   LVCFMT_CENTER,  80);
    m_ListMsgHook.InsertColumn(3, L"进程Id",    LVCFMT_RIGHT,   60);
    m_ListMsgHook.InsertColumn(4, L"线程Id",    LVCFMT_RIGHT,   60);
    m_ListMsgHook.InsertColumn(5, L"申请进程路径", LVCFMT_LEFT,  400);

    return TRUE;  
}

BOOL CMsgHook::DumpKernelMemory(ULONG pvAddress, ULONG cbDump, PVOID Buffer)
{
    if (theDriver.m_hDriver == NULL || 
        theDriver.m_hDriver == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }
    ULONG DumpAddress = (ULONG)pvAddress;
    ULONG BytesReturned;
    BytesReturned = theDriver.IoControl(IOCTL_DUMP_KERNEL_MEMORY,
                                        &DumpAddress, sizeof(ULONG),
                                        Buffer, cbDump);
    if (BytesReturned != cbDump)  return FALSE;
    return TRUE;
}

ULONG CMsgHook::LocateSharedInfo()
{
    ULONG pfnUserRegisterWowHandlers = (ULONG)
        GetProcAddress(GetModuleHandle(L"USER32.dll"), "UserRegisterWowHandlers");

    for (ULONG i = pfnUserRegisterWowHandlers; i < pfnUserRegisterWowHandlers + 0x1000; i++)
    {
        if ( *(PWORD)(i + 0 * 7) == 0x40C7 &&
             *(PWORD)(i + 1 * 7) == 0x40C7 &&
             *(PWORD)(i + 2 * 7) == 0x40C7 &&
             *(PWORD)(i + 3 * 7) == 0x40C7 &&
             *(PBYTE)(i + 4 * 7) == 0xB8 )
        {
            return *(PULONG)(i + 4 * 7 + 1);
        }
    }
    return 0;
}

void CMsgHook::OnMenuRefreshMsgHook()
{
    m_ListMsgHook.DeleteAllItems();
    if (m_pSharedInfo == NULL)
    {
        m_pSharedInfo = LocateSharedInfo();
        if (m_pSharedInfo == NULL)  return;
    }

    WORD wVersion = GetCurrentOSVersion();

    HANDLE      hProcess = GetCurrentProcess();
    SHAREDINFO  SharedInfo = {0};
    SERVERINFO  ServerInfo = {0};
    DWORD       dwReturnLength;

    ReadProcessMemory(hProcess, (PVOID)m_pSharedInfo, &SharedInfo, 
                      sizeof(SHAREDINFO), &dwReturnLength);
    ReadProcessMemory(hProcess, (PVOID)SharedInfo.psi, &ServerInfo, 
                      sizeof(SERVERINFO), &dwReturnLength);

    PHANDLEENTRY    pHandleEntries = NULL;
    PHANDLEENTRY    pHandleEntryTemp = NULL;
    PVOID           Buffer = NULL;
    HOOK_INFO       HookInfo = {0};
    CLIENT_ID       ClientId;
    MSG_HOOK_INFO   MsgHookInfo = {0};
    
    ServerInfo.cHandleEntries = ServerInfo.cHandleEntries & 0x0000FFFF;
    pHandleEntries = (PHANDLEENTRY)VirtualAlloc(NULL, ServerInfo.cHandleEntries * sizeof(HANDLEENTRY), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    //pHandleEntries = (PHANDLEENTRY)GlobalAlloc(GPTR, ServerInfo.cHandleEntries * sizeof(HANDLEENTRY));
    if (pHandleEntries == NULL)  return;
    pHandleEntryTemp = pHandleEntries;

    ReadProcessMemory(hProcess, (PVOID)SharedInfo.aheList, pHandleEntries, 
                      ServerInfo.cHandleEntries * sizeof(HANDLEENTRY), &dwReturnLength);

    Buffer = GlobalAlloc(GPTR, sizeof(HOOK_INFO));

    for (; pHandleEntryTemp < pHandleEntries + ServerInfo.cHandleEntries; pHandleEntryTemp ++)
    {
        if (pHandleEntryTemp->bType == TYPE_HOOK)
        {
            BOOL ret = FALSE; 
            ret = DumpKernelMemory((ULONG)pHandleEntryTemp->phead, sizeof(HOOK_INFO), Buffer);
            if (ret == FALSE)   continue;

            RtlCopyMemory(&HookInfo, Buffer, sizeof(HOOK_INFO));

            // 填充钩子信息到 MsgHookInfo 中
            MsgHookInfo.hHook = HookInfo.hHook;
            MsgHookInfo.HookType = HookInfo.HookType;
            MsgHookInfo.offPfn = HookInfo.OffPfn;

            ret = DumpKernelMemory((ULONG)HookInfo.Win32Thread, sizeof(ULONG), &MsgHookInfo.EThread);
            if (ret == FALSE)   continue;

            ULONG offset_Thread_ClientId;
            ULONG offset_Thread_ThreadsProcess;
            ULONG EProcess;

            switch (wVersion)
            {
            case VER_WINXP:
            case VER_WXPSP1:
            case VER_WXPSP2:
            case VER_WXPSP3:
                offset_Thread_ClientId = 0x1ec;
                offset_Thread_ThreadsProcess = 0x220;
                break;
            case VER_WINDOWS7:
                offset_Thread_ClientId = 0x22c;
                offset_Thread_ThreadsProcess = 0x150;
                break;
            }
            ret = DumpKernelMemory(MsgHookInfo.EThread + offset_Thread_ClientId, 
                                   sizeof(CLIENT_ID), &ClientId);
            if (ret == FALSE)   continue;
            ret = DumpKernelMemory(MsgHookInfo.EThread + offset_Thread_ThreadsProcess, 
                                   sizeof(ULONG), &EProcess);
            if (ret == FALSE || EProcess == 0)   continue;

            //////////////////////////////////////////////////////////////////////////
            WCHAR szHookType[32] = {0};
            WCHAR szProcessId[32] = {0};
            WCHAR szThreadId[32] = {0};
            WCHAR szHandle[32] = {0};
            WCHAR szRoutineAddr[16] = {0};
            WCHAR szImagePath[MAX_PATH] = {0};

            theDriver.IoControl(IOCTL_GET_PROCESS_IMAGE_PATH,
                                &EProcess, sizeof(ULONG),
                                szImagePath, 260 * 2);

            MsgHookInfo.ProcessId = (ULONG)ClientId.UniqueProcess;
            MsgHookInfo.ThreadId = (ULONG)ClientId.UniqueThread;

            wsprintf(szThreadId, L"%d", MsgHookInfo.ThreadId);
            wsprintf(szProcessId, L"%d", MsgHookInfo.ProcessId);
            wsprintf(szHandle, L" 0x%08X", MsgHookInfo.hHook);
            wsprintf(szRoutineAddr, L"0x%08X", MsgHookInfo.offPfn);

            if (MsgHookInfo.HookType < 15)
                wcscpy_s(szHookType, 32, HookTypeString[MsgHookInfo.HookType+1]);
            else
                wcscpy_s(szHookType, 32, L"UnKnown");

            int nItemNum = m_ListMsgHook.GetItemCount();
            m_ListMsgHook.InsertItem(nItemNum, szHandle);
            m_ListMsgHook.SetItemText(nItemNum, 1, szHookType);
            m_ListMsgHook.SetItemText(nItemNum, 2, szRoutineAddr);
            m_ListMsgHook.SetItemText(nItemNum, 3, szProcessId);
            m_ListMsgHook.SetItemText(nItemNum, 4, szThreadId);
            m_ListMsgHook.SetItemText(nItemNum, 5, szImagePath);

            m_ListMsgHook.SetItemData(nItemNum, (DWORD)MsgHookInfo.hHook);
        }
    }

    GlobalFree(Buffer);
    GlobalFree(pHandleEntries);

    wsprintf(StatusBuffer, L"消息钩子 :: %d", m_ListMsgHook.GetItemCount());
    theStatus->SetWindowText(StatusBuffer);
}

void CMsgHook::OnMenuUnhook()
{
    POSITION pos = m_ListMsgHook.GetFirstSelectedItemPosition();

    if (pos)
    {
        int nItem = m_ListMsgHook.GetNextSelectedItem(pos);
        HHOOK hHook = (HHOOK)m_ListMsgHook.GetItemData(nItem);
        UnhookWindowsHookEx(hHook);
    }
}

void CMsgHook::OnMenuLocate2File()
{
    // TODO: 在此添加命令处理程序代码
}

void CMsgHook::OnNMRClickListMsghook(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

    CPoint pt;
    GetCursorPos(&pt);
    CMenu menu;
    menu.LoadMenu(IDR_MENU3_MSGHOOK);
    CMenu* pMenu = menu.GetSubMenu(0);
    pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, this);

    *pResult = 0;
}
// CSystemRoutine 对话框

IMPLEMENT_DYNAMIC(CSystemRoutine, CDialog)

CSystemRoutine::CSystemRoutine(CWnd* pParent /*=NULL*/)
	: CDialog(CSystemRoutine::IDD, pParent)
{

}

CSystemRoutine::~CSystemRoutine()
{
}

void CSystemRoutine::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSystemRoutine, CDialog)
END_MESSAGE_MAP()

