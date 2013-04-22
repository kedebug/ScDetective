
#ifndef _PAGE1_H_
#define _PAGE1_H_

// CPage1 对话框

class CPage1 : public CDialog
{
    DECLARE_DYNAMIC(CPage1)

public:
    CPage1(CWnd* pParent = NULL);   // 标准构造函数
    virtual ~CPage1();

    // 对话框数据
    enum { IDD = IDD_SSDT };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    DECLARE_MESSAGE_MAP()

public:
    CListCtrl m_ListSsdt;
    WCHAR StatusBuffer[128];
    DWORD  SsdtCurrentAddress[500];
    PDWORD pSsdtNativeAddress;
    PSSDT_NAME pSsdtName;
    DWORD NumOfNativeAddress;
    DWORD NumOfFunName;

public:
    virtual BOOL OnInitDialog();

    afx_msg void OnMenuRefresh();
    afx_msg void OnNMRClickListSsdt(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnMenuSsdtUnhook();
    afx_msg void OnMenuSsdtUnhookAll();
    afx_msg void OnMenuShowAttribute();
    afx_msg void OnMenuLocateToFile();
    afx_msg void OnNMCustomdrawSsdtListColor(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
};

#endif


