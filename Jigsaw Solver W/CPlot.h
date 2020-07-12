#pragma once
#include <afxwin.h>

#define	PLOT_CLASSNAME		_T("MFCPlotCtrl")

class CPlot :
	public CWnd
{
	DECLARE_DYNAMIC(CPlot)

public:
	CPlot();
	~CPlot();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	BOOL Create(CWnd* pParentWnd, const RECT& rect, UINT nID, DWORD dwStyle = WS_VISIBLE)
	{
		return CWnd::Create(PLOT_CLASSNAME, _T(""), dwStyle, rect, pParentWnd, nID);
	}

	BOOL RegisterWindowClass();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

public:

	void Erase(bool bHold);
	void* Plot(const Curve& curve, COLORREF color = 0, int width = 1);
	void Delete(void* item);
	void Text(std::string text, int x, int y);
	void Unhold();


private:
	void Repaint();

	bool						m_bHold;
	std::list<Curve>			m_Curves;			// Set of curves to be plotted
	std::list<COLORREF>			m_Colors;
	std::list<int>				m_Widths;
	CBrush						m_brushBG;

	std::vector<std::string>	m_Strings;
	std::vector<CPoint>			m_stringLocations;
};

