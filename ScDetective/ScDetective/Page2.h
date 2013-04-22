
#ifndef __PAGE2_H__
#define __PAGE2_H__

// CPage2 对话框

class CPage2 : public CDialog
{
    DECLARE_DYNAMIC(CPage2)

public:
    CPage2(CWnd* pParent = NULL);   // 标准构造函数
    virtual ~CPage2();

    // 对话框数据
    enum { IDD = IDD_SSDTSHADOW };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();

private:
    CListCtrl m_ListSsdtShadow;
    WCHAR StatusBuffer[128];
    PDWORD pShadowSsdtNativeAddress;
    DWORD NumberOfNativeAddress;
    DWORD ShadowSsdtCurrentAddress[1024]; 
public:
    afx_msg void OnMenuRefreshSsdtShadow();
    afx_msg void OnMenuUnhook();
    afx_msg void OnMenuUnhookAll();
    afx_msg void OnMenuShowAttribute();
    afx_msg void OnMenuLocateToFile();
    afx_msg void OnNMCustomdrawListShadowSsdt(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMRClickListShadowSsdt(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
};

#endif


