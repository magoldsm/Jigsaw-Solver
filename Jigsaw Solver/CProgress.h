#pragma once

class CProgressBar
{
public:
	CProgressBar() : m_Percent(0.0), m_Bars(nullptr)
	{
		m_Time.QuadPart = 0;
	}
	CProgressBar(int n) : m_Percent(0.0)
	{
		m_Time.QuadPart = 0;
		m_Bars = new CProgressBar[n];
	}
	~CProgressBar() 
	{
		if (m_Bars) 
			delete[] m_Bars; 
		m_Bars = nullptr; 
	}

	void MakeSubbars(int n) 
	{
		m_Bars = new CProgressBar[n]; 
	}

	void Start(bool bStart);


	double			m_Percent;
	std::string		m_Text;
	LARGE_INTEGER	m_Time;
	CProgressBar*	m_Bars;
};


class CProgress
{
public:
	CProgress() : m_Bars(nullptr), m_hWndGUI(NULL)
	{
	}
	CProgress(int n)
	{
		m_Bars = new CProgressBar[n];
	}
	~CProgress()
	{
		if (m_Bars)
			delete[] m_Bars;
		m_Bars = nullptr;
	}

	CProgressBar*	m_Bars;

	HWND			m_hWndGUI;			// If we're in a GUI, we can use this handle to send update messages

	CProgressBar& operator[](int n) = delete;
	CProgressBar& operator()(int n) { return m_Bars[n]; }
	CProgressBar& operator()(int n, int m) { return m_Bars[n].m_Bars[m]; }

	void RestartReport(int n, bool bStart);				// Set bStart=1 to start a bar, 0 to reset it
	void RestartReport(int n, int m, bool bStart);		// Set bStart=1 to start a bar, 0 to reset it
	void UpdateReport();								// Update bars

	LRESULT Plot(const Curve& curve, COLORREF color = 0, int width = 1);
	void Erase(bool bHold = false);
	void Delete(LRESULT item);
	void Text(std::string text, int x, int y);
	void Unhold();
	void NewScores();

	void SavePuzzle();

private:

};

#define	PROGRESS_EUCLID		0
#define	PROGRESS_BIVERTEX	1
#define	PROGRESS_PLACING	2
#define	PROGRESS_COMPARING	3,0
#define	PROGRESS_CHECKING	3,1

#define	N_PROGRESS			4

#define	WM_PROGRESS			(WM_USER+1)
#define	WM_ERASE			(WM_USER+2)
#define	WM_PLOT				(WM_USER+3)
#define	WM_DELETE			(WM_USER+4)
#define	WM_TEXT				(WM_USER+5)
#define	WM_SAVE				(WM_USER+6)
#define	WM_UNHOLD			(WM_USER+7)
#define	WM_NEW_SCORE		(WM_USER+8)

extern CProgress Progress;
extern LARGE_INTEGER liStart, liFrequency, liSG, liEuclid, liBVD, liTotal;
