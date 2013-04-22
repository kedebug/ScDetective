// Page5.cpp : 实现文件
//

#include "stdafx.h"

extern CDriver theDriver;
extern CStatic* theStatus;
// CPage5 对话框

IMPLEMENT_DYNAMIC(CPage5, CDialog)

CPage5::CPage5(CWnd* pParent /*=NULL*/)
	: CDialog(CPage5::IDD, pParent)
{
    memset(StatusBuffer, 0, 128 * 2);
}

CPage5::~CPage5()
{
}

void CPage5::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST5, m_ListDriver);
}


BEGIN_MESSAGE_MAP(CPage5, CDialog)
    ON_WM_SHOWWINDOW()
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST5, &CPage5::OnNMCustomdrawListDrivers)
    ON_COMMAND(ID_MENU_32786, &CPage5::OnMenuRefreshDrivers)
    ON_NOTIFY(NM_RCLICK, IDC_LIST5, &CPage5::OnNMRClickListDrivers)
END_MESSAGE_MAP()


// CPage5 消息处理程序

BOOL CPage5::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_ListDriver.SetExtendedStyle(m_ListDriver.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_ListDriver.InsertColumn( 0, L"驱动名",     LVCFMT_CENTER, 80, -1);
    m_ListDriver.InsertColumn( 1, L"基地址",     LVCFMT_CENTER, 80, -1);
    m_ListDriver.InsertColumn( 2, L"映像大小",   LVCFMT_CENTER, 80, -1);
    m_ListDriver.InsertColumn( 3, L"驱动对象",   LVCFMT_CENTER, 80, -1);
    m_ListDriver.InsertColumn( 4, L"驱动路径",   LVCFMT_LEFT, 240, -1);  
    m_ListDriver.InsertColumn( 5, L"服务名",     LVCFMT_LEFT, 70, -1);
    m_ListDriver.InsertColumn( 6, L"隐藏",      LVCFMT_CENTER, 45, -1);
    m_ListDriver.InsertColumn( 7, L"文件厂商",   LVCFMT_LEFT, 110, -1);

    return TRUE;  
}

void CPage5::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CDialog::OnShowWindow(bShow, nStatus);

    if (bShow) {
        OnMenuRefreshDrivers();
    }
    theStatus->SetWindowText(StatusBuffer);
}

void CPage5::OnNMCustomdrawListDrivers(NMHDR *pNMHDR, LRESULT *pResult)
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
        if (m_ListDriver.GetItemText(nItem, 7, Buffer, 64) == 0)  
            clrNewTextColor = RGB(0, 0, 255);
        if (StrStr(Buffer, L"Microsoft ") == 0)
            clrNewTextColor = RGB(0, 0, 255);
        m_ListDriver.GetItemText(nItem, 6, Buffer, 64);
        if (StrStr(Buffer, L"是"))  clrNewTextColor = RGB(255, 0, 0);

        if( nItem % 2 == 0) clrNewBkColor = RGB(240, 240, 240);	  
        else  clrNewBkColor = RGB(255, 255, 255);	  

        pLVCD->clrText = clrNewTextColor;
        pLVCD->clrTextBk = clrNewBkColor;
        *pResult = CDRF_DODEFAULT;
    }
}

void CPage5::OnMenuRefreshDrivers()
{
    m_ListDriver.DeleteAllItems();
    if (theDriver.m_hDriver == NULL || 
        theDriver.m_hDriver == INVALID_HANDLE_VALUE)
    {
        return;
    }

    DWORD BytesReturned;
    ULONG NeedLen;
    PDRIVER_INFO DriverInformation = NULL;
    PVOID AllocBuffer;
    ULONG NumberOfDrivers;
    ULONG Number = 0;
    ULONG NumberOfHidden = 0;
    HANDLE hWaitEvent[2];

    hWaitEvent[0] = CreateEvent(NULL, FALSE, FALSE, NULL);

    BytesReturned = theDriver.IoControl(IOCTL_GET_DRIVER_OBJECT, 
                                        hWaitEvent, 2 * sizeof(HANDLE), 
                                        NULL, 0);
    WaitForSingleObject(hWaitEvent[0], INFINITE);
    CloseHandle(hWaitEvent[0]);

    BytesReturned = theDriver.IoControl(IOCTL_GET_DRIVER_OBJECT, 
                                        NULL, 0, 
                                        &NeedLen, sizeof(ULONG));

    AllocBuffer = GlobalAlloc(GPTR, NeedLen);
    DriverInformation = (PDRIVER_INFO)AllocBuffer;

    if (DriverInformation != NULL)
    {
        BytesReturned = theDriver.IoControl(IOCTL_GET_DRIVER_OBJECT, 
                                            NULL, 0, 
                                            DriverInformation, 
                                            NeedLen);
        NumberOfDrivers = NeedLen / sizeof(DRIVER_INFO);
    }
    else return ;

    WCHAR   szName[64];
    WCHAR   szDriverBase[16];
    WCHAR   szDriverSize[16];
    WCHAR   szDriverObject[16];
    WCHAR   szDriverPath[260];
    WCHAR   szImagePath[260];
    WCHAR   szServiceName[64];
    WCHAR   szHidden[16];
    WCHAR   szCorporation[128];

    for (ULONG i = 0; i < NumberOfDrivers; i++, DriverInformation++)
    {   
        memset(szName,          0, 64 * 2);
        memset(szDriverBase,    0, 16 * 2);
        memset(szDriverSize,    0, 16 * 2);
        memset(szDriverObject,  0, 16 * 2);
        memset(szDriverPath,    0, 260 * 2);
        memset(szImagePath,     0, 260 * 2);
        memset(szServiceName,   0, 64 * 2);
        memset(szHidden,        0, 16 * 2);
        memset(szCorporation,   0, 128 * 2);

        wsprintf(szDriverPath, L"%s", DriverInformation->ImagePath);
        wsprintf(szServiceName, L"%s", DriverInformation->ServiceName);
        wsprintf(szDriverSize, L"0x%08X", DriverInformation->DriverSize);
        wsprintf(szDriverBase, L"0x%08X", DriverInformation->ImageBase);

        if (DriverInformation->DriverObject)
            wsprintf(szDriverObject, L"0x%08X", DriverInformation->DriverObject);
        else  wcscpy_s(szDriverObject, 16, L"-");

        if (DriverInformation->bHidden) {
            wcscpy_s(szHidden, 16, L"是");
            NumberOfHidden ++;
        } else {
            wcscpy_s(szHidden, 16, L"-");
        }
        
        char path[260], win32path[260];
        WideCharToMultiByte(CP_ACP, 0, szDriverPath, 260, path, 260, NULL, NULL);
        ModifyFileImagePath(path, win32path, 260);
        MultiByteToWideChar(CP_ACP, 0, win32path, 260, szImagePath, 260);
        GetFileCorporation(szImagePath, szCorporation);
        GetFileNameByImagePath(szImagePath, szName);

        int nItemNum = m_ListDriver.GetItemCount();
        m_ListDriver.InsertItem(nItemNum, szName);
        m_ListDriver.SetItemText(nItemNum, 1, szDriverBase);
        m_ListDriver.SetItemText(nItemNum, 2, szDriverSize);
        m_ListDriver.SetItemText(nItemNum, 3, szDriverObject);
        m_ListDriver.SetItemText(nItemNum, 4, szImagePath);
        m_ListDriver.SetItemText(nItemNum, 5, szServiceName);
        m_ListDriver.SetItemText(nItemNum, 6, szHidden);
        m_ListDriver.SetItemText(nItemNum, 7, szCorporation);
    }
    wsprintf(StatusBuffer, L"共找到驱动模块 :: %d - 隐藏模块 :: %d", NumberOfDrivers, NumberOfHidden);
    theStatus->SetWindowText(StatusBuffer);
    GlobalFree(AllocBuffer);
}

void CPage5::OnNMRClickListDrivers(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    if (pNMListView->iItem != -1 && pNMListView->iSubItem != -1)
    {
        CPoint pt;
        GetCursorPos(&pt);
        CMenu menu;
        menu.LoadMenu(IDR_MENU5_DRIVER);
        CMenu* pMenu = menu.GetSubMenu(0);
        pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, this);
    }
    *pResult = 0;
}
