
// ScDetect.cpp : 定义应用程序的类行为。
//

#include "stdafx.h"
#include "ScDetective.h"
#include "ScDetectiveDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CScDetectApp

BEGIN_MESSAGE_MAP(CScDetectiveApp, CWinApp)
    ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CScDetectApp 构造

CScDetectiveApp::CScDetectiveApp()
{
    // TODO: 在此处添加构造代码，
    // 将所有重要的初始化放置在 InitInstance 中
}

//////////////////////////////////////////////////////////////////////////
//  全局变量
//////////////////////////////////////////////////////////////////////////
CScDetectiveApp theApp;                         // 唯一的一个 CScDetectApp 对象

CDriver theDriver;                              // 全局设备对象

#define LOAD_DRIVER

//////////////////////////////////////////////////////////////////////////

// CScDetectApp 初始化

BOOL CScDetectiveApp::InitInstance()
{
    WORD wVersion;
    wVersion = GetCurrentOSVersion();
    switch (wVersion)
    {
    case VER_WINXP:
    case VER_WXPSP1:
    case VER_WXPSP2:
    case VER_WXPSP3:
    case VER_WINDOWS7:
        break;
    default:
        MessageBox(NULL, L"本程序不支持您所使用的操作系统！", NULL, MB_OK);
        return FALSE;
    }

    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    // 将它设置为包括所有要在应用程序中使用的
    // 公共控件类。
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();

    SetRegistryKey(_T("应用程序向导生成的本地应用程序"));
    
    //////////////////////////////////////////////////////////////////////////
    // 加载驱动
#ifdef LOAD_DRIVER
    theDriver.LoadDriver(L"ScDetective.sys", L"ScDetective");
    ULONG ProcessId = GetCurrentProcessId();
    if (theDriver.m_hDriver && theDriver.m_hSCM) {
        theDriver.IoControl(IOCTL_PROTECT_MYSELF, &ProcessId, sizeof(ULONG), NULL, 0);
    }
    CloseMyHandles(ProcessId);
#endif
    
    CScDetectiveDlg dlg;
    m_pMainWnd = &dlg;
    INT_PTR nResponse = dlg.DoModal();
    if (nResponse == IDOK)
    {
        // TODO: 在此放置处理何时用
        //  “确定”来关闭对话框的代码
    }
    else if (nResponse == IDCANCEL)
    {
        // TODO: 在此放置处理何时用
        //  “取消”来关闭对话框的代码
    }

#ifdef LOAD_DRIVER
    // 卸载前要清理
    theDriver.IoControl(IOCTL_EXIT_PROCESS, NULL, 0, NULL, 0);
    // 驱动卸载
    theDriver.UnloadDriver();
#endif
    
    // 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
    //  而不是启动应用程序的消息泵。
    return FALSE;
}
