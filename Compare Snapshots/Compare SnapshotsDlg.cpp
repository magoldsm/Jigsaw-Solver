
// Compare SnapshotsDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Compare Snapshots.h"
#include "Compare SnapshotsDlg.h"
#include "CSnapshot.h"
#include "../Jigsaw Solver W/CPScoreDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CCompareSnapshotsDlg dialog



CCompareSnapshotsDlg::CCompareSnapshotsDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_COMPARESNAPSHOTS_DIALOG, pParent)
	, m_strLeftFile(_T(""))
	, m_strRightFile(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCompareSnapshotsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_LEFT_FILE, m_strLeftFile);
	DDX_Text(pDX, IDC_RIGHT_FILE, m_strRightFile);
}

BEGIN_MESSAGE_MAP(CCompareSnapshotsDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_COMPARE, &CCompareSnapshotsDlg::OnBnClickedCompare)
	ON_BN_CLICKED(IDC_SCORES, &CCompareSnapshotsDlg::OnBnClickedScores)
END_MESSAGE_MAP()


// CCompareSnapshotsDlg message handlers

BOOL CCompareSnapshotsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCompareSnapshotsDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCompareSnapshotsDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCompareSnapshotsDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

class Snapshot
{
public:

};

void CCompareSnapshotsDlg::OnBnClickedCompare()
{
	UpdateData();

	CSnapshot snapLeft(m_strLeftFile);
	CSnapshot snapRight(m_strRightFile);

	snapLeft.ReadIn();
	snapRight.ReadIn();

	snapLeft.Compare(snapRight);
}


#ifdef _DEBUG
void DebugOutput(const char* szFormat, ...)
{
	char szBuff[10240];
	va_list arg;
	va_start(arg, szFormat);
	_vsnprintf_s(szBuff, sizeof(szBuff), _TRUNCATE, szFormat, arg);
	va_end(arg);

	OutputDebugStringA(szBuff);
}
#endif


void CPScore::Display(double dP0)
{
	CPScoreDlg dlg(*this, dP0);

	dlg.DoModal();
}

void CPScore::Display(size_t nRow, size_t nCol, double dP0)
{
	__debugbreak();
}



void CCompareSnapshotsDlg::OnBnClickedScores()
{
	UpdateData();

	CSnapshot snapLeft(m_strLeftFile);
	snapLeft.ReadIn();

	snapLeft.DumpScores();
}
