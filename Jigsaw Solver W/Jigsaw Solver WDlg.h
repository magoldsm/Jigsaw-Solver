
// Jigsaw Solver WDlg.h : header file
//

#pragma once
#include "CParameters.h"
#include "CPlot.h"

class CProgressBar;

// CJigsawSolverWDlg dialog
class CJigsawSolverWDlg : public CDialogEx, public CParameters
{
// Construction
public:
	CJigsawSolverWDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_JIGSAWSOLVERW_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

public:

	CPlot m_Plot;
	CString m_strPlotLevel;
	BOOL m_bPause;

#define USE_WINTHREADS
#ifdef USE_WINTHREADS
	CWinThread* m_SolverThread;
#else
	std::thread* m_SolverThread;
#endif

	CString m_strPuzzleFile;
	CMFCEditBrowseCtrl m_PuzzleFile;
	CProgressCtrl m_barEuclidean;
	CProgressCtrl m_barBivertex;
	CProgressCtrl m_barPlacing;
	CProgressCtrl m_barComparing;
	CProgressCtrl m_barChecking;
	CStatic m_pctEuclidean;
	CStatic m_runtimeEuclidean;
	CStatic m_remainingEuclidean;
	CStatic m_pctBivertex;
	CStatic m_runtimeBivertex;
	CStatic m_remainingBivertex;
	CStatic m_pctPlacing;
	CStatic m_runtimePlacing;
	CStatic m_remainingPlacing;
	CStatic m_pctComparing;
	CStatic m_runtimeComparing;
	CStatic m_remainingComparing;
	CStatic m_pctChecking;
	CStatic m_runtimeChecking;
	CStatic m_remainingChecking;


	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnEnKillfocusJstar();
	afx_msg void OnBnClickedSolve();
	afx_msg void OnBnClickedPause();
	afx_msg void OnBnClickedResume();
	afx_msg LRESULT OnProgress(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnErase(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPlot(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDelete(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnText(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSave(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUnhold(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNewScore(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnConfirmPlacement(WPARAM wParam, LPARAM lParam);

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	void UpdateParameters();

	void UpdateBar(int idx, CProgressBar & bar);
	afx_msg void OnClose();
//	afx_msg void OnBnClickedCheck1();
	CListBox m_Scores;
	afx_msg void OnBnClickedLoadParams();
	afx_msg void OnBnClickedSaveParams();
};
