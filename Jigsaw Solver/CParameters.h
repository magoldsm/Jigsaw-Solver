#pragma once
class CParameters
{
public:
	CParameters();
	~CParameters();

	void SaveParams();

	void Serialize(CArchive& ar);

	char m_szPath[4096];

	int m_nSGOrder;
	int m_nSGWindow;
	int m_nLambda0;
	int m_nLambda1;
	int m_nAlpha;
	int m_nBeta;
	int m_nGamma;
	int m_nNu;
	double m_dRho;
	int m_nJmax;
	int m_nC1;
	int m_nC2;
	double m_dK1;
	double m_dK2;
	double m_dK4;
	double m_dEpsilon;
	double m_dEta1;
	double m_dEta2;
	double m_dQ1;
	double m_dQ2;
	double m_dQ2Star;
	double m_dQ3;
	int m_nJStar;

	double	m_P0[8];
	double	m_M0[8];
	double	m_MU0[8];
	double	m_K3[8];

	BOOL m_bPlotBVD;
	BOOL m_bShowPScores;
	BOOL m_bSave;
	BOOL m_bDumpFitBeforeQSort;

	char m_szPlotLevel[100];

private:
	HKEY	m_regKey;

	DWORD GetDwordParam(const char* name, DWORD def);
	void PutDwordParam(const char* name, DWORD value);
	void GetStringParam(const char* name, char* buff, size_t sz, const char* def);
	void PutStringParam(const char* name, const char* value);
};

extern CParameters* pParams;				// The one and only!

