
// ScDetectDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ScDetective.h"
#include "ScDetectiveDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern CDriver theDriver;

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

    // 对话框数据
    enum { IDD = IDD_ABOUTBOX };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    // 实现
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CScDetectDlg 对话框

CStatic* theStatus;


CScDetectiveDlg::CScDetectiveDlg(CWnd* pParent /*=NULL*/)
: CDialog(CScDetectiveDlg::IDD, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CScDetectiveDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TAB_PAGE, theTable);
    DDX_Control(pDX, IDC_STATUS_TEXT, m_Status);
}

BEGIN_MESSAGE_MAP(CScDetectiveDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    //}}AFX_MSG_MAP
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_PAGE, &CScDetectiveDlg::OnTcnSelectChangeTabPage)
END_MESSAGE_MAP()


// CScDetectDlg 消息处理程序

BOOL CScDetectiveDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // 将“关于...”菜单项添加到系统菜单中。

    // IDM_ABOUTBOX 必须在系统命令范围内。
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
    //  执行此操作
    SetIcon(m_hIcon, TRUE);			// 设置大图标
    SetIcon(m_hIcon, FALSE);		// 设置小图标

    // TODO: 在此添加额外的初始化代码
    ShowWindow(SW_NORMAL);

    theStatus = &m_Status;

    thePageArray[0] = &m_Page1;
    thePageArray[1] = &m_Page2;
    thePageArray[2] = &m_Page3;
    thePageArray[3] = &m_Page4;
    thePageArray[4] = &m_Page5;
    thePageArray[5] = &m_Page6;

    theTable.MoveWindow(CRect(0,0,830,20), FALSE);

    theTable.InsertItem(0, L"ssdt");
    thePageArray[0]->Create(IDD_SSDT, this);

    theTable.InsertItem(1, L"ssdt shadow");
    thePageArray[1]->Create(IDD_SSDTSHADOW, this);

    theTable.InsertItem(2, L"应用层钩子");
    thePageArray[2]->Create(IDD_PAGE_R3HOOK, this);

    theTable.InsertItem(3, L"进程管理");
    thePageArray[3]->Create(IDD_PAGE_PROCESS, this);

    theTable.InsertItem(4, L"驱动信息");
    thePageArray[4]->Create(IDD_PAGE_DRIVEROBJECT, this);

    theTable.InsertItem(5, L"文件管理");
    thePageArray[5]->Create(IDD_PAGE_FILE, this);

    theTable.SetCurSel(0);

    RECT rect;
    this->GetClientRect(&rect);
    rect.left   +=  5;
    rect.top    +=  22;
    rect.bottom +=  20;

    for (int i = 0; i < SC_PAGE_NUM; i++)
    {
        thePageArray[i]->MoveWindow(&rect);
    }

    thePageArray[0]->ShowWindow(SW_SHOW);

    this->GetClientRect(&rect);
    rect.left   +=  20;
    rect.bottom +=  5;
    rect.top    =   rect.bottom - 30;

    m_Status.MoveWindow(&rect);

    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CScDetectiveDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CScDetectiveDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // 用于绘制的设备上下文

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 使图标在工作区矩形中居中
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // 绘制图标
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CScDetectiveDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CScDetectiveDlg::OnTcnSelectChangeTabPage(NMHDR *pNMHDR, LRESULT *pResult)
{
    int nCurSelect = theTable.GetCurSel();
    ShowPageX(nCurSelect);
    *pResult = 0;
}

VOID CScDetectiveDlg::ShowPageX(int nIndex)
{
    for (int i = 0; i < SC_PAGE_NUM; i++)
    {
        thePageArray[i]->ShowWindow(SW_HIDE);
    }
    thePageArray[nIndex]->BringWindowToTop();
    thePageArray[nIndex]->ShowWindow(SW_SHOWNORMAL);
    thePageArray[nIndex]->UpdateWindow();
}

