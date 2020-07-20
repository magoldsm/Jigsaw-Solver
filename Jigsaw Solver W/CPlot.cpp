#include "pch.h"
#include "CPlot.h"

using namespace std;
using namespace Eigen;

IMPLEMENT_DYNAMIC(CPlot, CWnd)


CPlot::CPlot()
	: m_bHold(false)
{
	RegisterWindowClass();
}


CPlot::~CPlot()
{
}


BEGIN_MESSAGE_MAP(CPlot, CWnd)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
END_MESSAGE_MAP()


BOOL CPlot::RegisterWindowClass()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (!(::GetClassInfo(hInst, PLOT_CLASSNAME, &wndcls)))
	{
		m_brushBG.CreateSolidBrush(RGB(255, 255, 255));

		// otherwise we need to register a new class
		wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
		wndcls.hInstance = hInst;
		wndcls.hIcon = NULL;
		wndcls.hCursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground = (HBRUSH)m_brushBG;
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = PLOT_CLASSNAME;

		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CPlot::OnEraseBkgnd(CDC* pDC)
{
	return CWnd::OnEraseBkgnd(pDC);
}

static
void BoundingBox(list<Curve>& curves, double& xScale, double& yScale, double& minX, double& minY)
{
	size_t sz = curves.size();

	minX = MAXINT;
	minY = MAXINT;

	double maxX = -MAXINT, maxY = -MAXINT;

	for (std::list<Curve>::iterator it = curves.begin(); it != curves.end(); ++it)
	{
		const Curve& curve = *it;

		double mx = curve.col(0).maxCoeff();
		double mn = curve.col(0).minCoeff();
		minX = std::min(mn, minX);
		maxX = std::max(mx, maxX);

		mx = curve.col(1).maxCoeff();
		mn = curve.col(1).minCoeff();
		minY = std::min(mn, minY);
		maxY = std::max(mx, maxY);
	}

	xScale = maxX - minX;
	yScale = maxY - minY;
}

using itCurve = std::list<Curve>::iterator;
using itColor = std::list<COLORREF>::iterator;
using itWidth = std::list<int>::iterator;

void CPlot::OnPaint()
{
	CPaintDC dc(this); 

	double xScale, yScale, minX, minY;
	BoundingBox(m_Curves, xScale, yScale, minX, minY);

	if (xScale > yScale)
		yScale = xScale;
	else
		xScale = yScale;

	CRect rectClient;
	GetClientRect(&rectClient);

	xScale /= rectClient.Width();
	yScale /= rectClient.Height();

	itCurve itcurve = m_Curves.begin();
	itColor itcolor = m_Colors.begin();
	itWidth itwidth = m_Widths.begin();

	while (itcurve != m_Curves.end())
	{
		Curve& curve = *itcurve;

		auto width = *itwidth;
		bool bClose = width > 1;
		width = std::abs(width);

		CPen pen;
		pen.CreatePen(PS_SOLID, width, *itcolor);
		auto oldPen = dc.SelectObject(&pen);

		int x = (int)((curve(0, 0) - minX) / xScale);
		int y = rectClient.Height() - (int)((curve(0, 1) - minY) / yScale);

		dc.MoveTo(x, y);
//		DebugOutput("  0   %3d %3d\n", x, y);

		size_t sz = curve.rows();

		int prevx = x;
		int prevy = y;

		for (int j = 1; j < sz; j++)
		{
			int x = (int)((curve(j, 0) - minX) / xScale);
			int y = rectClient.Height() - (int)((curve(j, 1) - minY) / yScale);

			double dist = sqrt((x - prevx)*(x - prevx) + (y - prevy)*(y - prevy)) * xScale;
//			DebugOutput("d = %.2f\n", dist);

//			DebugOutput("%3d   %3d %3d\n", j, x, y);

			if (dist > 10)
				dc.MoveTo(x, y);
			else
				dc.LineTo(x, y);
			prevx = x;
			prevy = y;
		}

		double dist = sqrt((x - prevx)*(x - prevx) + (y - prevy)*(y - prevy))* xScale;
//		DebugOutput("%3d   %3d %3d\n", j, x, y);
		if (bClose && dist <= 10)
			dc.LineTo(x, y);

		dc.SelectObject(oldPen);

		itcurve++;
		itcolor++;
		itwidth++;
	}

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
	LOGFONT lf;
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 18;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);

	for (int i = 0; i < m_Strings.size(); i++)
	{
		CString str(m_Strings[i].c_str());
		CPoint& pt = m_stringLocations[i];
		int x = (int)((pt.x - minX) / xScale);
		int y = rectClient.Height() - (int)((pt.y - minY) / yScale);
		dc.ExtTextOutW(x, y, 0, NULL, str, NULL);
//		dc.TextOut(x, y, str);
	}

	dc.SelectObject(pOldFont);
}


BOOL CPlot::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Add your specialized code here and/or call the base class

	return CWnd::PreCreateWindow(cs);
}

void CPlot::Erase(bool bHold)
{
//	DebugOutput("CPlot::Erase\n");

	m_Curves.resize(0);
	m_Colors.resize(0);
	m_Widths.resize(0);
	Repaint();

	m_bHold = bHold;
}

void* CPlot::Plot(const Curve & curve, COLORREF color, int width)
{
	if (width > 127)
		width = width - 256;
	m_Curves.push_back(curve);
	m_Colors.push_back(color);
	m_Widths.push_back(width);
	Repaint();

	itCurve itc = m_Curves.end();
	itc--;

//	DebugOutput("CPlot::Plot:   0x%x (%d)\n", &(*itc), m_Curves.size());

	return &(*itc);
}

void CPlot::Delete(void * item)
{
//	DebugOutput("CPlot::Delete: 0x%x (%d)\n", item, m_Curves.size()-1);

	itCurve itcurve = m_Curves.begin();
	itColor itcolor = m_Colors.begin();
	itWidth itwidth = m_Widths.begin();

	while (itcurve != m_Curves.end())
	{
		if (item == &(*itcurve))
		{
			m_Curves.erase(itcurve);
			m_Colors.erase(itcolor);
			m_Widths.erase(itwidth);
			return;
		}

		itcurve++;
		itcolor++;
		itwidth++;
	}
}

void CPlot::Text(std::string text, int x, int y)
{
	m_Strings.push_back(text);
	m_stringLocations.push_back(CPoint(x, y));

	Repaint();
}

void CPlot::Repaint()
{
	if (m_bHold)
		return;

	CRect rect;
	GetWindowRect(&rect);
	GetParent()->ScreenToClient(&rect);
	GetParent()->InvalidateRect(&rect, FALSE);

}

void CPlot::Unhold()
{
	m_bHold = false;
	Repaint();
}