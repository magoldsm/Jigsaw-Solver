// CPScoreDlg.cpp : implementation file
//

#include "pch.h"
#include "Jigsaw Solver W.h"
#include "CPScoreDlg.h"
#include "afxdialogex.h"
#include "GridCtrl.h"
#include "SignatureSimilarity.h"

// CPScoreDlg dialog

IMPLEMENT_DYNAMIC(CPScoreDlg, CDialogEx)

CPScoreDlg::CPScoreDlg(CPScore& pscore, double dP0, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PSCORE_GRID, pParent)
	, m_PScore(pscore)
	, m_nRow(-1)
	, m_nCol(-1)
	, m_dP0(dP0)
{

}

CPScoreDlg::CPScoreDlg(CPScore& pscore, int row, int col, double dP0, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PSCORE_GRID, pParent)
	, m_PScore(pscore)
	, m_nRow(row)
	, m_nCol(col)
	, m_dP0(dP0)
{

}

CPScoreDlg::~CPScoreDlg()
{
}

void CPScoreDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_GridControl(pDX, IDC_GRID, m_Grid);
}


BEGIN_MESSAGE_MAP(CPScoreDlg, CDialogEx)
	ON_NOTIFY(NM_DBLCLK, IDC_GRID, OnGridDblClick)
	ON_NOTIFY(NM_CLICK, IDC_GRID, OnGridClick)
	ON_BN_CLICKED(IDC_COPY, &CPScoreDlg::OnBnClickedCopy)
END_MESSAGE_MAP()


// CPScoreDlg message handlers

void CPScoreDlg::InitPieces()
{
	m_Grid.SetRowCount((int)m_PScore.GetSize() + 1);
	m_Grid.SetColumnCount((int)m_PScore.GetSize() + 1);

	for (int i = 0; i < m_PScore.GetSize(); i++)
	{
		TCHAR buff[_MAX_ITOSTR_BASE10_COUNT];
		_itow_s<_MAX_ITOSTR_BASE10_COUNT>(i, buff, 10);

		m_Grid.SetItemText(0, i+1, buff);
		m_Grid.SetItemText(i + 1, 0, buff);

		for (int j = 0; j < m_PScore.GetSize(); j++)
		{
			Eigen::MatrixXd& a = m_PScore(i, j);
			CString strVal;
			strVal.Format(_T("%dx%d"), a.rows(), a.cols());
			m_Grid.SetItemText(i + 1, j + 1, strVal);
		}
	}
}


void CPScoreDlg::InitArcs()
{
	Eigen::MatrixXd& a = m_PScore(m_nRow, m_nCol);

	m_Grid.SetRowCount((int)a.rows() + 1);
	m_Grid.SetColumnCount((int)a.cols() + 1);

	CString strVal;
	strVal.Format(_T("%d, %d"), m_nRow, m_nCol);
	m_Grid.SetItemText(0, 0, strVal);

	for (int i = 0; i < a.rows(); i++)
	{
		TCHAR buff[_MAX_ITOSTR_BASE10_COUNT];
		_itow_s<_MAX_ITOSTR_BASE10_COUNT>(i, buff, 10);

		m_Grid.SetItemText(i + 1, 0, buff);
	}
	
	for (int i = 0; i < a.cols(); i++)
	{
		TCHAR buff[_MAX_ITOSTR_BASE10_COUNT];
		_itow_s<_MAX_ITOSTR_BASE10_COUNT>(i, buff, 10);

		m_Grid.SetItemText(0, i + 1, buff);
	}

	for (int i = 0; i < a.rows(); i++)
	{
		for (int j = 0; j < a.cols(); j++)
		{
			double dval = a(i, j);
			CString strVal;
			strVal.Format(_T("%.4f"), dval);
			m_Grid.SetItemText(i + 1, j + 1, strVal);
			if (dval >= m_dP0)
				m_Grid.SetItemBkColour(i+1, j+1, RGB(255, 192, 192));
		}
	}
}


BOOL CPScoreDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_Grid.SetFixedRowCount(1);
	m_Grid.SetFixedColumnCount(1);

	if (m_nRow == -1)
		InitPieces();
	else
		InitArcs();


	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}


void CPScoreDlg::OnGridDblClick(NMHDR *pNotifyStruct, LRESULT* /*pResult*/)
{
	NM_GRIDVIEW* pItem = (NM_GRIDVIEW*)pNotifyStruct;
	DebugOutput(("Double Clicked on row %d, col %d\n"), pItem->iRow-1, pItem->iColumn-1);

	CPScoreDlg dlg(m_PScore, pItem->iRow-1, pItem->iColumn-1, m_dP0);

	dlg.DoModal();
}

void CPScoreDlg::OnGridClick(NMHDR *pNotifyStruct, LRESULT* /*pResult*/)
{
	NM_GRIDVIEW* pItem = (NM_GRIDVIEW*)pNotifyStruct;
	DebugOutput(("Clicked on row %d, col %d\n"), pItem->iRow, pItem->iColumn);
}


void CPScoreDlg::OnBnClickedCopy()
{
	m_Grid.OnEditCopy();
}
