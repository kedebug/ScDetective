// Page2.cpp : 实现文件
//

#include "stdafx.h"

extern CDriver theDriver;
extern CStatic* theStatus;
// CPage2 对话框

IMPLEMENT_DYNAMIC(CPage2, CDialog)

CPage2::CPage2(CWnd* pParent /*=NULL*/)
: CDialog(CPage2::IDD, pParent)
{
    memset(StatusBuffer, 0, 128 * sizeof(WCHAR));
    memset(ShadowSsdtCurrentAddress, 0, 1024 * sizeof(DWORD));
    pShadowSsdtNativeAddress = NULL;
    NumberOfNativeAddress = 0;
    pShadowSsdtNativeAddress = GetShadowSsdtNativeFunAddresses(&NumberOfNativeAddress);
}

CPage2::~CPage2()
{
    if (pShadowSsdtNativeAddress)
    {
        GlobalFree(pShadowSsdtNativeAddress);
    }
}

void CPage2::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST2, m_ListSsdtShadow);
}


BEGIN_MESSAGE_MAP(CPage2, CDialog)
    ON_COMMAND(ID_MENU_32776, &CPage2::OnMenuRefreshSsdtShadow)
    ON_COMMAND(ID_MENU_32777, &CPage2::OnMenuUnhook)
    ON_COMMAND(ID_MENU_32778, &CPage2::OnMenuUnhookAll)
    ON_COMMAND(ID_MENU_32779, &CPage2::OnMenuShowAttribute)
    ON_COMMAND(ID_MENU_32780, &CPage2::OnMenuLocateToFile)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST2, &CPage2::OnNMCustomdrawListShadowSsdt)
    ON_NOTIFY(NM_RCLICK, IDC_LIST2, &CPage2::OnNMRClickListShadowSsdt)
    ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()


// CPage2 消息处理程序

BOOL CPage2::OnInitDialog()
{
    CDialog::OnInitDialog();
    
    // TODO:  在此添加额外的初始化
    m_ListSsdtShadow.SetExtendedStyle(m_ListSsdtShadow.GetExtendedStyle() | 
                                      LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_ListSsdtShadow.InsertColumn( 0, L"服务号",     LVCFMT_CENTER, 60, -1);
    m_ListSsdtShadow.InsertColumn( 1, L"服务函数名", LVCFMT_LEFT, 170, -1);
    m_ListSsdtShadow.InsertColumn( 2, L"当前地址",   LVCFMT_CENTER, 100, -1);
    m_ListSsdtShadow.InsertColumn( 3, L"Hook",      LVCFMT_CENTER, 80, -1);
    m_ListSsdtShadow.InsertColumn( 4, L"原始地址",   LVCFMT_CENTER, 100, -1);  
    m_ListSsdtShadow.InsertColumn( 5, L"模块名",     LVCFMT_LEFT, 250, -1);

    OnMenuRefreshSsdtShadow();

    return TRUE;  // return TRUE unless you set the focus to a control
    // 异常: OCX 属性页应返回 FALSE
}

void CPage2::OnMenuRefreshSsdtShadow()
{
    m_ListSsdtShadow.DeleteAllItems();

    if (theDriver.m_hDriver == NULL || 
        theDriver.m_hDriver == INVALID_HANDLE_VALUE)
    {
        return;
    }

    DWORD BytesReturned;
    ULONG NeedLen;
    ULONG Number = 0;
    PSSDT_ADDRESS pCurrentSsdtAddr = NULL;
    SSDT_INFO ShadowSsdtInfo = { 0 };
    UINT nChanged = 0;
    BOOL ret = FALSE;

    BytesReturned = theDriver.IoControl(IOCTL_GET_SSDTSHADOW, 
                                        NULL, 0, 
                                        &NeedLen, 
                                        sizeof(ULONG)
                                        );
    pCurrentSsdtAddr = (PSSDT_ADDRESS)GlobalAlloc(GPTR, NeedLen);

    if (pCurrentSsdtAddr != NULL)
    {
        BytesReturned = theDriver.IoControl(IOCTL_GET_SSDTSHADOW, 
                                            NULL, 0, 
                                            pCurrentSsdtAddr, 
                                            NeedLen);
        Number = NeedLen / sizeof(SSDT_ADDRESS);
        if (Number > NumberOfNativeAddress)
            Number = NumberOfNativeAddress;
    }
    else return ;
   
    WORD wVersion = GetCurrentOSVersion();

    for (ULONG i = 0; i < Number; i++)
    {
        ShadowSsdtInfo.NativeAddress  = pShadowSsdtNativeAddress[i];
        ShadowSsdtInfo.CurrentAddress = pCurrentSsdtAddr[i].FunAddress;
        ShadowSsdtInfo.Index = i;
        ShadowSsdtInfo.Hideflag = FALSE;

        ShadowSsdtCurrentAddress[i] = pCurrentSsdtAddr[i].FunAddress;

        if (ShadowSsdtInfo.CurrentAddress != ShadowSsdtInfo.NativeAddress)
        {
            ShadowSsdtInfo.Hideflag = TRUE;
        }

        __try
        {
            ZeroMemory(ShadowSsdtInfo.FunName, 64);
            if (wVersion == VER_WXPSP1 || 
                wVersion == VER_WXPSP2 ||
                wVersion == VER_WXPSP3) {
                strcpy_s(ShadowSsdtInfo.FunName, 64, XPFunName[i]);
            }

            if (wVersion == VER_W2K3||
                wVersion == VER_W2K3SP2) {
                strcpy_s(ShadowSsdtInfo.FunName, 64, W2K3FunName[i]);
            }

            if (wVersion == VER_VISTA11 ||
                wVersion == VER_VISTASP1 ||
                wVersion == VER_VISTAULT) {
                strcpy_s(ShadowSsdtInfo.FunName, 64, VistaFunName[i]);
            }

            if (wVersion == VER_WINDOWS7) {
                strcpy_s(ShadowSsdtInfo.FunName, 64, Win7FunName[i]);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            strcpy_s(ShadowSsdtInfo.FunName, 64, "未知");
        }

        char szPath[MAX_PATH] = { 0 };

        ret = GetKernelModuleNameByAddress(szPath, MAX_PATH, ShadowSsdtInfo.CurrentAddress, TRUE);
        if (ret == TRUE)
        {
            // lstrcpyA(SsdtInfo.ImagePath, szPath);
            ModifyFileImagePath(szPath, ShadowSsdtInfo.ImagePath, 260);
        }

        WCHAR szIndex[16];
        WCHAR szServiceName[64];
        WCHAR szCurrentAddr[16];
        WCHAR szNativeAddr[16];
        WCHAR szImagePath[MAX_PATH];

        wsprintf(szIndex, L"%03d", i);
        wsprintf(szServiceName, L"%S", ShadowSsdtInfo.FunName);
        wsprintf(szCurrentAddr, L"0x%08X", ShadowSsdtInfo.CurrentAddress);
        wsprintf(szNativeAddr, L"0x%08X", ShadowSsdtInfo.NativeAddress);
        MultiByteToWideChar(CP_ACP, 0, ShadowSsdtInfo.ImagePath, MAX_PATH, szImagePath, MAX_PATH);

        int nItemNum = m_ListSsdtShadow.GetItemCount();
        m_ListSsdtShadow.InsertItem(nItemNum, szIndex);
        m_ListSsdtShadow.SetItemText(nItemNum, 1, szServiceName);
        m_ListSsdtShadow.SetItemText(nItemNum, 2, szCurrentAddr);
        m_ListSsdtShadow.SetItemText(nItemNum, 4, szNativeAddr);
        m_ListSsdtShadow.SetItemText(nItemNum, 5, szImagePath);

        if (ShadowSsdtInfo.Hideflag) {
            ++ nChanged;
            m_ListSsdtShadow.SetItemText(nItemNum, 3, L"ssdt hook");
        } else {
            m_ListSsdtShadow.SetItemText(nItemNum, 3, L"-");
        }
    }

    wsprintf(StatusBuffer, L"shadow ssdt 函数 :: %d - 被挂钩函数 :: %d", NumberOfNativeAddress, nChanged);
    theStatus->SetWindowText(StatusBuffer);

    GlobalFree(pCurrentSsdtAddr);
}

void CPage2::OnMenuUnhook()
{
    SSDT_ADDRESS ShadowSsdtInfo = { 0 };
    WCHAR szBuffer[16] = { 0 };
    WCHAR szCurrentAddr[16] = { 0 };
    WCHAR szOrigAddr[16] = { 0 };
    WCHAR szIndex[16] = { 0 };
    ULONG CurrentAddress;
    ULONG OrigAddress;
    POSITION pos = m_ListSsdtShadow.GetFirstSelectedItemPosition();

    if (pos)
    {
        int nItem = m_ListSsdtShadow.GetNextSelectedItem(pos);
        m_ListSsdtShadow.GetItemText(nItem, 2, szCurrentAddr, 16);
        swscanf_s(szCurrentAddr + 2, L"%X", &CurrentAddress);
        m_ListSsdtShadow.GetItemText(nItem, 4, szOrigAddr, 16);
        swscanf_s(szOrigAddr + 2, L"%X", &OrigAddress);

        if (CurrentAddress == OrigAddress) goto theEnd;

        m_ListSsdtShadow.GetItemText(nItem, 0, szIndex, 16);
        ShadowSsdtInfo.nIndex = _wtol(szIndex);
        ShadowSsdtInfo.FunAddress = OrigAddress;

        ULONG BytesReturned = theDriver.IoControl(IOCTL_UNHOOK_SSDTSHADOW, 
                                                  &ShadowSsdtInfo, 
                                                  sizeof(SSDT_ADDRESS), 
                                                  NULL, 0);
    }
theEnd:
    OnMenuRefreshSsdtShadow();
}

void CPage2::OnMenuUnhookAll()
{
    SSDT_ADDRESS ShadowSsdtInfo = { 0 };
    WCHAR szBuffer[16];
    WCHAR szCurrentAddr[16];
    WCHAR szOrigAddr[16];
    WCHAR szIndex[16];
    ULONG CurrentAddress;
    ULONG OrigAddress;
    UINT  nCount = m_ListSsdtShadow.GetItemCount();

    for (UINT nItem = 0; nItem < nCount; nItem++)
    {
        memset(szBuffer,        0, 32);
        memset(szCurrentAddr,   0, 32);
        memset(szOrigAddr,      0, 32);
        memset(szIndex,         0, 32);

        m_ListSsdtShadow.GetItemText(nItem, 2, szCurrentAddr, 16);
        swscanf_s(szCurrentAddr + 2, L"%X", &CurrentAddress);
        m_ListSsdtShadow.GetItemText(nItem, 4, szOrigAddr, 16);
        swscanf_s(szOrigAddr + 2, L"%X", &OrigAddress);

        if (CurrentAddress == OrigAddress)  continue;

        m_ListSsdtShadow.GetItemText(nItem, 0, szIndex, 16);
        ShadowSsdtInfo.nIndex = _wtol(szIndex);
        ShadowSsdtInfo.FunAddress = OrigAddress;

        ULONG BytesReturned = theDriver.IoControl(IOCTL_UNHOOK_SSDTSHADOW, 
                                                  &ShadowSsdtInfo, 
                                                  sizeof(SSDT_ADDRESS), 
                                                  NULL, 0);
    }

    OnMenuRefreshSsdtShadow();
}

void CPage2::OnMenuShowAttribute()
{
    // TODO: 在此添加命令处理程序代码
}

void CPage2::OnMenuLocateToFile()
{
    // TODO: 在此添加命令处理程序代码
}

void CPage2::OnNMCustomdrawListShadowSsdt(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );

    // TODO: 在此添加控件通知处理程序代码
    *pResult = CDRF_DODEFAULT;;

    if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
    {
        *pResult = CDRF_NOTIFYITEMDRAW;
    }
    else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
    {
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
    }
    else if ( (CDDS_ITEMPREPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage )
    {
        COLORREF clrNewTextColor, clrNewBkColor;
        int nItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);

        //如果当前 Shadow SSDT地址与原始地址不一样,则显示字体为红色,否则为黑色
        if(pShadowSsdtNativeAddress[nItem] != ShadowSsdtCurrentAddress[nItem])    //设置为红色
        {
            clrNewTextColor = RGB(255, 0, 0);
        }
        else		//设置为黑色
        {
            clrNewTextColor = RGB(0, 0, 0);
        }
        //设置背景色
        if( nItem % 2 == 0)
        {
            clrNewBkColor = RGB(240, 240, 240);	    //偶数行背景色为灰色
        }
        else
        {
            clrNewBkColor = RGB(255, 255, 255);	    //奇数行背景色为白色
        }

        pLVCD->clrText = clrNewTextColor;
        pLVCD->clrTextBk = clrNewBkColor;

        *pResult = CDRF_DODEFAULT;
    }
}

void CPage2::OnNMRClickListShadowSsdt(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: 在此添加控件通知处理程序代码
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    // if (pNMListView->iItem != -1 && pNMListView->iSubItem != -1)
    {
        CPoint pt;
        GetCursorPos(&pt);
        CMenu menu;
        menu.LoadMenu(IDR_MENU2_SSDT_SHADOW);
        CMenu* pMenu = menu.GetSubMenu(0);
        pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, this);
    }
    *pResult = 0;
}

void CPage2::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CDialog::OnShowWindow(bShow, nStatus);

    // TODO: 在此处添加消息处理程序代码
    theStatus->SetWindowText(StatusBuffer);
}
