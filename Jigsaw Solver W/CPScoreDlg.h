#pragma once
#include "GridCtrl.h"

class CPScore;

// CPScoreDlg dialog

class CPScoreDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CPScoreDlg)

public:
	CPScoreDlg(CPScore& pscore, double dP0 = 1.0, CWnd* pParent = nullptr);   // standard constructor
	CPScoreDlg(CPScore& pscore, int row, int col, double dP0 = 1.0, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CPScoreDlg();

	CGridCtrl		m_Grid;
	CPScore&		m_PScore;
	int				m_nRow;
	int				m_nCol;
	double			m_dP0;

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PSCORE_GRID };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	void CPScoreDlg::OnGridDblClick(NMHDR *pNotifyStruct, LRESULT* /*pResult*/);
	void CPScoreDlg::OnGridClick(NMHDR *pNotifyStruct, LRESULT* /*pResult*/);
	void InitPieces();
	void InitArcs();
	afx_msg void OnBnClickedCopy();
};
