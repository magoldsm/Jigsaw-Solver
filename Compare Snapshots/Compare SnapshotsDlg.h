
// Compare SnapshotsDlg.h : header file
//

#pragma once


// CCompareSnapshotsDlg dialog
class CCompareSnapshotsDlg : public CDialogEx
{
// Construction
public:
	CCompareSnapshotsDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_COMPARESNAPSHOTS_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CString m_strLeftFile;
	CString m_strRightFile;
	afx_msg void OnBnClickedCompare();
	afx_msg void OnBnClickedScores();
};
