
// Jigsaw Solver WDlg.cpp : implementation file
//

#include "pch.h"
#include "Jigsaw Solver W.h"
#include "Jigsaw Solver WDlg.h"
#include "afxdialogex.h"
#include "CProgress.h"
#include "CParameters.h"
#include "Savitsy-Golay.h"
#include "Euclidean Signature.h"
#include "BivertexArcDecomposition.h"
#include "Utilities.h"
#include "SignatureSimilarity.h"
#include "CPlot.h"
#include "CPScoreDlg.h"

using namespace std;
using namespace cv;
using namespace Eigen;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

std::vector<CPiece> Pieces;
Eigen::VectorXd smoothVec, d1Vec, d2Vec;
double Dx, Dy, Dkappa, Dkappas;
double AverageLength;
long AverageSize;
CProgress Progress(N_PROGRESS);

void CProgress::UpdateReport()
{
	::PostMessage(m_hWndGUI, WM_PROGRESS, 0, 0L);
}


LRESULT CProgress::Plot(const Curve& curve, COLORREF color, int width)
{
	LRESULT lr = 0;

	if (curve.rows())
	{
	//	DebugOutput("Plot: Curve(%d), color:%x, width:%d\n", curve.rows(), color, width);

		lr = ::SendMessage(m_hWndGUI, WM_PLOT, color | ((width & 0xff) << 24), (LPARAM)&curve);
	}
	else
		__debugbreak();
	//Sleep(10);
	return lr;
}


void CProgress::Text(std::string text, int x, int y)
{
	LRESULT lr = ::SendMessage(m_hWndGUI, WM_TEXT, (WPARAM)(&CPoint(x, y)), (LPARAM)&text);
	//Sleep(10);
}


void CProgress::Erase(bool bHold)
{
	::SendMessage(m_hWndGUI, WM_ERASE, bHold, 0L);
}


void CProgress::Delete(LRESULT item)
{
	::SendMessage(m_hWndGUI, WM_DELETE, 0, item);
	//Sleep(10);
}


void CProgress::SavePuzzle()
{
	::SendMessage(m_hWndGUI, WM_SAVE, 0, 0L);
}

void CProgress::Unhold()
{
	::SendMessage(m_hWndGUI, WM_UNHOLD, 0, 0L);
}

void CProgress::NewScores()
{
	::SendMessage(m_hWndGUI, WM_NEW_SCORE, 0, 0L);
}

LRESULT CProgress::ConfirmPlacement(const char* szMessage)
{
	return ::SendMessage(m_hWndGUI, WM_CONFIRM_PLACEMENT, 0, (LPARAM) szMessage);
}

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


// CJigsawSolverWDlg dialog



CJigsawSolverWDlg::CJigsawSolverWDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_JIGSAWSOLVERW_DIALOG, pParent)
	, m_bPause(FALSE)
	, m_SolverThread(nullptr)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CJigsawSolverWDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PLOT, m_Plot);
	DDX_Text(pDX, IDC_SG_ORDER, m_nSGOrder);
	DDX_Text(pDX, IDC_SG_WINDOW, m_nSGWindow);
	DDX_Text(pDX, IDC_ALPHA, m_nAlpha);
	DDX_Text(pDX, IDC_BETA, m_nBeta);
	DDX_Text(pDX, IDC_GAMMA, m_nGamma);
	DDX_Text(pDX, IDC_C1, m_nC1);
	DDX_Text(pDX, IDC_C2, m_nC2);
	DDX_Text(pDX, IDC_K1, m_dK1);
	DDX_Text(pDX, IDC_K2, m_dK2);
	DDX_Text(pDX, IDC_K4, m_dK4);
	DDX_Text(pDX, IDC_LAMBDA0, m_nLambda0);
	DDX_Text(pDX, IDC_LAMBDA1, m_nLambda1);
	DDX_Text(pDX, IDC_NU, m_nNu);
	DDX_Text(pDX, IDC_EPSILON, m_dEpsilon);
	DDX_Text(pDX, IDC_RHO, m_dRho);
	DDX_Text(pDX, IDC_JMAX, m_nJmax);
	DDX_Text(pDX, IDC_ETA1, m_dEta1);
	DDX_Text(pDX, IDC_ETA2, m_dEta2);
	DDX_Text(pDX, IDC_Q1, m_dQ1);
	DDX_Text(pDX, IDC_Q2, m_dQ2);
	DDX_Text(pDX, IDC_Q2STAR, m_dQ2Star);
	DDX_Text(pDX, IDC_Q3, m_dQ3);
	DDX_Text(pDX, IDC_P0, m_P0[0]);
	DDX_Text(pDX, IDC_M0, m_M0[0]);
	DDX_Text(pDX, IDC_MU0, m_MU0[0]);
	DDX_Text(pDX, IDC_K30, m_K3[0]);
	DDX_Text(pDX, IDC_P1, m_P0[1]);
	DDX_Text(pDX, IDC_M1, m_M0[1]);
	DDX_Text(pDX, IDC_MU1, m_MU0[1]);
	DDX_Text(pDX, IDC_K31, m_K3[1]);
	DDX_Text(pDX, IDC_P2, m_P0[2]);
	DDX_Text(pDX, IDC_M2, m_M0[2]);
	DDX_Text(pDX, IDC_MU2, m_MU0[2]);
	DDX_Text(pDX, IDC_K32, m_K3[2]);
	DDX_Text(pDX, IDC_P3, m_P0[3]);
	DDX_Text(pDX, IDC_M3, m_M0[3]);
	DDX_Text(pDX, IDC_MU3, m_MU0[3]);
	DDX_Text(pDX, IDC_K33, m_K3[3]);
	DDX_Text(pDX, IDC_P4, m_P0[4]);
	DDX_Text(pDX, IDC_M4, m_M0[4]);
	DDX_Text(pDX, IDC_MU4, m_MU0[4]);
	DDX_Text(pDX, IDC_K34, m_K3[4]);
	DDX_Text(pDX, IDC_P5, m_P0[5]);
	DDX_Text(pDX, IDC_M5, m_M0[5]);
	DDX_Text(pDX, IDC_MU5, m_MU0[5]);
	DDX_Text(pDX, IDC_K35, m_K3[5]);
	DDX_Text(pDX, IDC_P6, m_P0[6]);
	DDX_Text(pDX, IDC_M6, m_M0[6]);
	DDX_Text(pDX, IDC_MU6, m_MU0[6]);
	DDX_Text(pDX, IDC_K36, m_K3[6]);
	DDX_Text(pDX, IDC_P7, m_P0[7]);
	DDX_Text(pDX, IDC_M7, m_M0[7]);
	DDX_Text(pDX, IDC_MU7, m_MU0[7]);
	DDX_Text(pDX, IDC_K37, m_K3[7]);
	DDX_Text(pDX, IDC_JSTAR, m_nJStar);
	if (!pDX->m_bSaveAndValidate)
		m_strPuzzleFile = m_szPath;
	DDX_Text(pDX, IDC_PUZZLE_FILE, m_strPuzzleFile);
	if (pDX->m_bSaveAndValidate)
		strcpy_s(m_szPath, sizeof(m_szPath), CT2A(m_strPuzzleFile));
	DDX_Control(pDX, IDC_EUCLIDEAN_SIGNATURES, m_barEuclidean);
	DDX_Control(pDX, IDC_BIVERTEX_ARC, m_barBivertex);
	DDX_Control(pDX, IDC_PLACING_PIECES, m_barPlacing);
	DDX_Control(pDX, IDC_COMPARING_PIECES, m_barComparing);
	DDX_Control(pDX, IDC_CHECKING_FITS, m_barChecking);
	DDX_Check(pDX, IDC_PLOT_BVD, m_bPlotBVD);
	DDX_Check(pDX, IDC_SHOW_PSCORES, m_bShowPScores);
	if (!pDX->m_bSaveAndValidate)
		m_strPlotLevel = m_szPlotLevel;
	DDX_CBString(pDX, IDC_PLOT_LEVEL, m_strPlotLevel);
	if (pDX->m_bSaveAndValidate)
		strcpy_s(m_szPlotLevel, sizeof(m_szPlotLevel), CT2A(m_strPlotLevel));
	DDX_Check(pDX, IDC_PAUSE, m_bPause);
	DDX_Check(pDX, IDC_SAVE_PROGRESS, m_bSave);
	DDX_Control(pDX, IDC_EUCLIDEAN_SIGNATURES_PCT, m_pctEuclidean);
	DDX_Control(pDX, IDC_EUCLIDEAN_SIGNATURES_RUNTIME, m_runtimeEuclidean);
	DDX_Control(pDX, IDC_EUCLIDEAN_SIGNATURES_REMAINING, m_remainingEuclidean);
	DDX_Control(pDX, IDC_BIVERTEX_ARC_PCT, m_pctBivertex);
	DDX_Control(pDX, IDC_BIVERTEX_ARC_RUNTIME, m_runtimeBivertex);
	DDX_Control(pDX, IDC_BIVERTEX_ARC_REMAINING, m_remainingBivertex);
	DDX_Control(pDX, IDC_PLACING_PIECES_PCT, m_pctPlacing);
	DDX_Control(pDX, IDC_PLACING_PIECES_RUNTIME, m_runtimePlacing);
	DDX_Control(pDX, IDC_PLACING_PIECES_REMAINING, m_remainingPlacing);
	DDX_Control(pDX, IDC_COMPARING_PIECES_PCT, m_pctComparing);
	DDX_Control(pDX, IDC_COMPARING_PIECES_RUNTIME, m_runtimeComparing);
	DDX_Control(pDX, IDC_COMPARING_PIECES_REMAINING, m_remainingComparing);
	DDX_Control(pDX, IDC_CHECKING_FITS_PCT, m_pctChecking);
	DDX_Control(pDX, IDC_CHECKING_FITS_RUNTIME, m_runtimeChecking);
	DDX_Control(pDX, IDC_CHECKING_FITS_REMAINING, m_remainingChecking);
	DDX_Check(pDX, IDC_DUMP_FIT_BEFORE_QSORT, m_bDumpFitBeforeQSort);
	DDX_Control(pDX, IDC_SCORES, m_Scores);
	DDX_Check(pDX, IDC_CONFORM_BEFORE_PLACING, m_bConfirmBeforePlacing);
	DDX_Control(pDX, IDC_PUZZLE_FILE, m_PuzzleFile);
}

BEGIN_MESSAGE_MAP(CJigsawSolverWDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_EN_KILLFOCUS(IDC_JSTAR, &CJigsawSolverWDlg::OnEnKillfocusJstar)
	ON_BN_CLICKED(IDC_SOLVE, &CJigsawSolverWDlg::OnBnClickedSolve)
	ON_MESSAGE(WM_PROGRESS, &CJigsawSolverWDlg::OnProgress)
	ON_MESSAGE(WM_ERASE, &CJigsawSolverWDlg::OnErase)
	ON_MESSAGE(WM_PLOT, &CJigsawSolverWDlg::OnPlot)
	ON_MESSAGE(WM_DELETE, &CJigsawSolverWDlg::OnDelete)
	ON_MESSAGE(WM_TEXT, &CJigsawSolverWDlg::OnText)
	ON_MESSAGE(WM_SAVE, &CJigsawSolverWDlg::OnSave)
	ON_MESSAGE(WM_UNHOLD, &CJigsawSolverWDlg::OnUnhold)
	ON_MESSAGE(WM_NEW_SCORE, &CJigsawSolverWDlg::OnNewScore)
	ON_MESSAGE(WM_CONFIRM_PLACEMENT, &CJigsawSolverWDlg::OnConfirmPlacement)
	ON_BN_CLICKED(IDC_PAUSE, &CJigsawSolverWDlg::OnBnClickedPause)
	ON_BN_CLICKED(IDC_RESUME, &CJigsawSolverWDlg::OnBnClickedResume)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_LOAD_PARAMS, &CJigsawSolverWDlg::OnBnClickedLoadParams)
	ON_BN_CLICKED(IDC_SAVE_PARAMS, &CJigsawSolverWDlg::OnBnClickedSaveParams)
END_MESSAGE_MAP()


// CJigsawSolverWDlg message handlers

struct BarControls
{
	CProgressCtrl		CJigsawSolverWDlg::*bar;
	CStatic				CJigsawSolverWDlg::*pct;
	CStatic				CJigsawSolverWDlg::*runtime;
	CStatic				CJigsawSolverWDlg::*remaining;
};

#define BC(x) \
	&CJigsawSolverWDlg::m_bar##x, \
	&CJigsawSolverWDlg::m_pct##x, \
	&CJigsawSolverWDlg::m_runtime##x, \
	&CJigsawSolverWDlg::m_remaining##x


static BarControls bc[] = {
	{ BC(Euclidean) },
	{ BC(Bivertex) },
	{ BC(Placing) },
	{ BC(Comparing) },
	{ BC(Checking) },
};

BOOL CJigsawSolverWDlg::OnInitDialog()
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

	Progress(3).MakeSubbars(2);
	Progress.m_hWndGUI = m_hWnd;	// Have the progress bar send us messages

	m_barEuclidean.SetRange(0, 1000);
	m_barBivertex.SetRange(0, 1000);
	m_barPlacing.SetRange(0, 1000);
	m_barComparing.SetRange(0, 1000);
	m_barChecking.SetRange(0, 1000);

	for (int idx = 0; idx < (sizeof(bc) / sizeof(bc[0])); idx++)
	{
		(this->*bc[idx].pct).SetWindowText(_T(""));
		(this->*bc[idx].runtime).SetWindowText(_T(""));
		(this->*bc[idx].remaining).SetWindowText(_T(""));
	}

	OnEnKillfocusJstar();

	CRect rect;
	m_Plot.GetWindowRect(&rect);
	ScreenToClient(&rect);
	InvalidateRect(&rect, FALSE);

	m_PuzzleFile.EnableFileBrowseButton(_T("csv"), _T("Puzzles (*.csv)|*.csv||"));

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CJigsawSolverWDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CJigsawSolverWDlg::OnPaint()
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
HCURSOR CJigsawSolverWDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

#define	CTRLS(n)	IDC_P##n, IDC_M##n, IDC_MU##n, IDC_K3##n

int Ctrls[][4] = {
	{ CTRLS(0) },
	{ CTRLS(1) },
	{ CTRLS(2) },
	{ CTRLS(3) },
	{ CTRLS(4) },
	{ CTRLS(5) },
	{ CTRLS(6) },
	{ CTRLS(7) },
};

void CJigsawSolverWDlg::OnEnKillfocusJstar()
{
	UpdateData(TRUE);

	for (int i = 0; i < 8; i++)
	{
		if (i < m_nJStar)
		{
			GetDlgItem(Ctrls[i][0])->ShowWindow(SW_SHOW);
			GetDlgItem(Ctrls[i][1])->ShowWindow(SW_SHOW);
			GetDlgItem(Ctrls[i][2])->ShowWindow(SW_SHOW);
			GetDlgItem(Ctrls[i][3])->ShowWindow(SW_SHOW);
		}
		else
		{
			GetDlgItem(Ctrls[i][0])->ShowWindow(SW_HIDE);
			GetDlgItem(Ctrls[i][1])->ShowWindow(SW_HIDE);
			GetDlgItem(Ctrls[i][2])->ShowWindow(SW_HIDE);
			GetDlgItem(Ctrls[i][3])->ShowWindow(SW_HIDE);
		}
	}
}


BOOL CJigsawSolverWDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UINT notificationCode = (UINT)HIWORD(wParam);

	// For List control I handle it in another way....
	if ((notificationCode == EN_KILLFOCUS) ||
		(notificationCode == LBN_KILLFOCUS) ||
		(notificationCode == CBN_KILLFOCUS) ||
		(notificationCode == NM_KILLFOCUS) ||
		(notificationCode == WM_KILLFOCUS)) {

		CWnd *pFocus = CWnd::GetFocus();
		// call to a static function 

		// If we are changing the focus to another
		// control of the same window... 

		if (pFocus && (pFocus->GetParent() == this))
		{
			// Ok, if the focus is not in the cancel button...
			if (pFocus->GetDlgCtrlID() != IDCANCEL) {
				UpdateData(TRUE);
				UpdateParameters();
			}
		}
	}

	return CDialogEx::OnCommand(wParam, lParam);
}


void CJigsawSolverWDlg::UpdateParameters()
{
	SaveParams();
}


void CJigsawSolverWDlg::OnBnClickedSolve()
{
	UpdateData();

#ifdef USE_WINTHREADS
	m_SolverThread = ::AfxBeginThread([](LPVOID pParam)->UINT {
		Solver(nullptr);
		return 0;
	}, nullptr);
#else
	m_SolverThread = new std::thread([](LPVOID pParam)->void {
		Solver((char*)nullptr);
	});
#endif
}


LRESULT CJigsawSolverWDlg::OnProgress(WPARAM wParam, LPARAM lParam)
{
	UpdateBar(0, Progress(PROGRESS_EUCLID));
	UpdateBar(1, Progress(PROGRESS_BIVERTEX));
	UpdateBar(2, Progress(PROGRESS_PLACING));
	UpdateBar(3, Progress(PROGRESS_COMPARING));
	UpdateBar(4, Progress(PROGRESS_CHECKING));

	return 0;
}


void CJigsawSolverWDlg::UpdateBar(int idx, CProgressBar & bar)
{
	int nLower, nUpper;
	(this->*bc[idx].bar).GetRange(nLower, nUpper);
	int nDiff = nUpper - nLower;
	int val = (int)((bar.m_Percent * nDiff) + nLower);

	(this->*bc[idx].bar).SetPos(val);

	CString str;
	str.Format(_T("%.1f%% Complete"), 100*bar.m_Percent);
	(this->*bc[idx].pct).SetWindowText(str);

	if (bar.m_Percent < 1.0 && bar.m_Time.QuadPart > 0)
	{
		LARGE_INTEGER now;
		::QueryPerformanceCounter(&now);
		long elapsed = (long)((now.QuadPart - bar.m_Time.QuadPart) / (1.0*liFrequency.QuadPart));

		str = CvtElapsedTime(elapsed);
		(this->*bc[idx].runtime).SetWindowText(str + _T(" Running"));

		if (bar.m_Percent == 0)
			(this->*bc[idx].remaining).SetWindowText(_T(""));
		else
		str = CvtElapsedTime((long)(elapsed / bar.m_Percent - elapsed));
		(this->*bc[idx].remaining).SetWindowText(str + _T(" Remaining"));
	}
	else
		(this->*bc[idx].remaining).SetWindowText(_T(""));
}

LRESULT CJigsawSolverWDlg::OnErase(WPARAM wParam, LPARAM lParam)
{
	m_Plot.Erase(wParam);
	return 0;
}


LRESULT CJigsawSolverWDlg::OnPlot(WPARAM wParam, LPARAM lParam)
{
	return (LRESULT) m_Plot.Plot(*((Curve*)lParam), wParam&0xffffff, (wParam>>24)&0xff);
}

LRESULT CJigsawSolverWDlg::OnText(WPARAM wParam, LPARAM lParam)
{
	CPoint pt = *(CPoint*)(wParam);
	std::string str = *(std::string*)(lParam);
	m_Plot.Text(str, pt.x, pt.y);
	return 0;
}

LRESULT CJigsawSolverWDlg::OnUnhold(WPARAM wParam, LPARAM lParam)
{
	m_Plot.Unhold();
	return 0;
}


LRESULT CJigsawSolverWDlg::OnNewScore(WPARAM wParam, LPARAM lParam)
{
	size_t szPlacements = Placements.size();
	int nElements = m_Scores.GetCount();

	while (szPlacements > nElements)
	{
		CString str;
		CPlacement& p = Placements[nElements++];
		str.Format(_T("%3d  %.3f  %.3f  %.3f  %.3f"), p.m_nPiece, p.m_Score(0), p.m_Score(1), p.m_Score(2), p.m_Score(3), p.m_Score(4));
		m_Scores.AddString(str);
	}

	return 0;
}


LRESULT CJigsawSolverWDlg::OnConfirmPlacement(WPARAM wParam, LPARAM lParam)
{
	const char* pszMessage = (const char*)lParam;
	CString strMess;
	strMess.Format(_T("%hs\nAccept Placement?"), pszMessage);

	auto res = ::AfxMessageBox(strMess, MB_YESNO | MB_ICONQUESTION);
	return res;
}


LRESULT CJigsawSolverWDlg::OnSave(WPARAM wParam, LPARAM lParam)
{
	CString strFilename;
	strFilename.Format(_T("CurrentPuzzleData%04d.spz"), Placements.size());

	CFile file(strFilename, CFile::modeWrite | CFile::typeBinary | CFile::modeCreate);
	CArchive ar(&file, CArchive::store);

	ar << AverageLength << AverageSize << Dx << Dy << Dkappa << Dkappas;

	pParams->Serialize(ar);

	ar << Placements.size();
	for (CPlacement& p : Placements) p.Serialize(ar);

	ar << Tracker.size();
	for (CTracker& t : Tracker) t.Serialize(ar);

	ar << Pieces.size();
	for (CPiece& p : Pieces) p.Serialize(ar);

	PScores.Serialize(ar);

	ar << Fits.size();
	for (CFit& f : Fits) f.Serialize(ar);

	return 0;
}

LRESULT CJigsawSolverWDlg::OnDelete(WPARAM wParam, LPARAM lParam)
{
	m_Plot.Delete((void*)lParam);
	return 0;
}

void CPScore::Display(double dP0)
{
	CPScoreDlg dlg(*this, dP0);

	dlg.DoModal();
}

void CPScore::Display(size_t nRow, size_t nCol, double dP0)
{
	__debugbreak();
}

void CJigsawSolverWDlg::OnBnClickedPause()
{
	UpdateData();
	
	if (m_bPause)
		Pauser.Lock();
	else
		Pauser.Unlock();
}


void CJigsawSolverWDlg::OnBnClickedResume()
{
	UpdateData();

	CFileDialog dlg(TRUE, _T(".spz"), NULL, OFN_FILEMUSTEXIST| OFN_HIDEREADONLY| OFN_NOCHANGEDIR, _T("Saved Puzzles (*.spz)|*.spz||"));

	if (dlg.DoModal() != IDOK)
		return;

	CString str = dlg.GetPathName();
	static CStringA strPathname(str);
	const char* szPath = strPathname;

#ifdef USE_WINTHREADS
	m_SolverThread = ::AfxBeginThread([](LPVOID pParam)->UINT {
		Solver((char*) pParam);
		return 0;
	}, (LPVOID) szPath);
#else
	m_SolverThread = new std::thread([szPath](LPVOID pParam)->void {
		Solver((char*)szPath);
	});
#endif

}


void CJigsawSolverWDlg::OnClose()
{
#ifdef USE_WINTHREADS
	if (m_SolverThread)
		m_SolverThread->SuspendThread();
#else
#endif

	__super::OnClose();
}



void CJigsawSolverWDlg::OnBnClickedLoadParams()
{
	CFileDialog dlg(TRUE, _T(".prm"), NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR, _T("Saved Puzzles (*.prm)|*.prm||"));

	if (dlg.DoModal() != IDOK)
		return;

	CFile file(dlg.GetPathName(), CFile::modeRead | CFile::typeBinary);
	CArchive ar(&file, CArchive::load);

	CParameters::Serialize(ar);

	UpdateData(FALSE);

	OnEnKillfocusJstar();
}


void CJigsawSolverWDlg::OnBnClickedSaveParams()
{
	UpdateData(TRUE);

	CFileDialog dlg(FALSE, _T(".prm"), NULL, OFN_HIDEREADONLY | OFN_NOCHANGEDIR, _T("Saved Puzzles (*.prm)|*.prm||"));

	if (dlg.DoModal() != IDOK)
		return;

	CFile file(dlg.GetPathName(), CFile::modeWrite | CFile::modeCreate | CFile::typeBinary);
	CArchive ar(&file, CArchive::store);

	CParameters::Serialize(ar);

}
