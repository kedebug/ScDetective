#pragma once
#include "afxcmn.h"


// CPageFile 对话框

class CPageFile : public CDialog
{
	DECLARE_DYNAMIC(CPageFile)

public:
	CPageFile(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPageFile();

// 对话框数据
	enum { IDD = IDD_PAGE_FILE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
    CTreeCtrl m_TreeFile;
    CListCtrl m_ListFile;
    HTREEITEM m_hRoot;
    CImageList m_ImageList;

private:
    BOOL JudgeSubDirectory(PWCHAR pszPath);
    void ListDirectory(HTREEITEM hItem);
    void DeleteChildren(HTREEITEM hItem);
    void GetItemPath(HTREEITEM hItem, PWCHAR pszPath, ULONG cbszPath);

public:
    virtual BOOL OnInitDialog();
    afx_msg void OnTvnSelchangedTreeFile(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
};
