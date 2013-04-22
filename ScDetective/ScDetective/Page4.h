
#ifndef _PAGE4_H_
#define _PAGE4_H_

// CProcessThreads 对话框

class CProcessThreads : public CDialog
{
    DECLARE_DYNAMIC(CProcessThreads)

public:
    CProcessThreads(CWnd* pParent = NULL);   // 标准构造函数
    virtual ~CProcessThreads();

    // 对话框数据
    enum { IDD = IDD_PROCESS_THREADS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    DECLARE_MESSAGE_MAP()
private:
    CListCtrl m_ListThreads;

private:
    void ShowThreads(ULONG EProcess);
public:
    virtual BOOL OnInitDialog();
};

//////////////////////////////////////////////////////////////////////////
// CProcessModules 对话框

class CProcessModules : public CDialog
{
    DECLARE_DYNAMIC(CProcessModules)

public:
    CProcessModules(CWnd* pParent = NULL);   // 标准构造函数
    virtual ~CProcessModules();

    // 对话框数据
    enum { IDD = IDD_PROCESS_MODULES };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    DECLARE_MESSAGE_MAP()
public:
    CListCtrl m_ListModules;
private:
    void ShowModules(ULONG EProcess);
private:
    virtual BOOL OnInitDialog();
};

//////////////////////////////////////////////////////////////////////////
// CProcessHandles 对话框

class CProcessHandles : public CDialog
{
    DECLARE_DYNAMIC(CProcessHandles)

public:
    CProcessHandles(CWnd* pParent = NULL);   // 标准构造函数
    virtual ~CProcessHandles();

    // 对话框数据
    enum { IDD = IDD_PROCESS_HANDLES };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    DECLARE_MESSAGE_MAP()
public:
    CListCtrl m_ListHandles;
};


// CPage4 对话框

class CPage4 : public CDialog
{
	DECLARE_DYNAMIC(CPage4)

public:
	CPage4(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPage4();

// 对话框数据
	enum { IDD = IDD_PAGE_PROCESS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
    CListCtrl m_ListProcess;
    CProcessThreads m_ProcessThreads;
    CProcessModules m_ProcessModules;
    CProcessHandles m_ProcessHandles;
    WCHAR StatusBuffer[128];

private:
    virtual BOOL OnInitDialog();
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
public:
    afx_msg void OnNMRClickListProcess(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMCustomdrawListProcess(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnMenuRefreshProcess();
    afx_msg void OnMenuShowProcessModules();
    afx_msg void OnMenuShowProcessThreads();
    afx_msg void OnMenuShowProcessHandles();
    afx_msg void OnMenuLocate2File();
};

#endif

