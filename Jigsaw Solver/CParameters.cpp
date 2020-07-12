#include "pch.h"
#include "CParameters.h"
#include "Utilities.h"

using namespace std;

//#define LARGE_PUZZLE			// >= 100
//#define MEDIUM_PUZZLE			// >= 69
#define SMALL_PUZZLE			// < 69

constexpr auto APP = "Jigsaw Solver";

#ifdef SMALL_PUZZLE
#define	N_PARAMS	2

static int defaultP[] = { 940, 900 };
static int defaultM[] = { 2000, 2000 };
static int defaultMu[] = { 600, 600 };
static int defaultK3[] = { 707, 2000 };

#define	LAMBDA0		30
#define	LAMBDA1		10
#define	Q_1			900
#define	Q_2			200
#define	Q_2STAR		300
#define	Q_3			0
#endif // SMALL_PUZZLE

#ifdef MEDIUM_PUZZLE
#define	N_PARAMS	2

static int defaultP[] = { 940, 900 };
static int defaultM[] = { 3000, 2000 };
static int defaultMu[] = { 600, 600 };
static int defaultK3[] = { 1118, 2000 };

#define	LAMBDA0		20
#define	LAMBDA1		10
#define	Q_1			900
#define	Q_2			200
#define	Q_2STAR		300
#define	Q_3			0
#endif // MEDIUM_PUZZLE

#ifdef LARGE_PUZZLE
#define	N_PARAMS	1

static int defaultP[] = { 940 };
static int defaultM[] = { 2000 };
static int defaultMu[] = { 600 };
static int defaultK3[] = { 707 };

#define	LAMBDA0		20
#define	LAMBDA1		8
#define	Q_1			976
#define	Q_2			240
#define	Q_2STAR		300
#define	Q_3			29
#endif // LARGE_PUZZLE

DWORD
CParameters::GetDwordParam(const char* name, DWORD def)
{
	DWORD dwType;
	DWORD data;
	DWORD cbData = sizeof(data);

	LSTATUS res = ::RegGetValueA(m_regKey, APP, name, RRF_RT_DWORD, &dwType, &data, &cbData);
	if (res != ERROR_SUCCESS)
		return def;

	return data;
}

void
CParameters::PutDwordParam(const char* name, DWORD value)
{
	LSTATUS res = RegSetKeyValueA(m_regKey, APP, name, REG_DWORD, &value, sizeof(value));
}

void CParameters::GetStringParam(const char* name, char* buff, size_t sz, const char* def)
{
	DWORD dwSize = (DWORD) sz;
	DWORD dwType;

	LSTATUS res = ::RegGetValueA(m_regKey, APP, name, RRF_RT_REG_SZ, &dwType, buff, &dwSize);
	if (res != ERROR_SUCCESS)
		strcpy_s(buff, sizeof(buff), def);
}

void CParameters::PutStringParam(const char * name, const char* value)
{
	LSTATUS res = ::RegSetKeyValueA(m_regKey, APP, name, REG_SZ, value, (DWORD) strlen(value)+1);
}

CParameters::CParameters()
	: m_bSave(false)
	, m_bDumpFitBeforeQSort(FALSE)
{
	LSTATUS res = ::RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\WinStock Software\\", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &m_regKey, NULL);

	GetStringParam("File", m_szPath, sizeof(m_szPath), "");

	m_nSGOrder = GetDwordParam("sgOrder", 2);			// 5 for raw data
	m_nSGWindow = GetDwordParam("sgWindow", 10);		// 50 for raw data
	m_nLambda0 = GetDwordParam("lambda0", LAMBDA0);
	m_nLambda1 = GetDwordParam("lambda1", LAMBDA1);
	m_nAlpha = GetDwordParam("alpha", 2);
	m_nBeta = GetDwordParam("beta", 5);
	m_nGamma = GetDwordParam("gamma", 5);
	m_nNu = GetDwordParam("nu", 3);
	m_dRho = GetDwordParam("rho", 333) / 1000.0;
	m_nC1 = GetDwordParam("C1", 1000);
	m_nC2 = GetDwordParam("C2", 2);
	m_dK1 = GetDwordParam("K1", 15);
	m_dK2 = GetDwordParam("K2", 4);
	m_dK4 = GetDwordParam("K4", 500)/1000.0;
	m_dEpsilon = GetDwordParam("epsilon", 100) / 1000000.0;
	m_dEta1 = GetDwordParam("eta1", 1000) / 1000.0;
	m_dEta2 = GetDwordParam("eta2", 1500) / 1000.0;
	m_dQ1 = GetDwordParam("Q1", Q_1)/1000.0;
	m_dQ2 = GetDwordParam("Q2", Q_2) / 1000.0;
	m_dQ2Star = GetDwordParam("Q2Star", Q_2STAR) / 1000.0;
	m_dQ3 = GetDwordParam("Q3", Q_3) / 1000.0;
	m_dK2 = GetDwordParam("K2", 4);
	m_nJmax = GetDwordParam("jMax", 3);

	m_nJStar = GetDwordParam("nParams", N_PARAMS);

	int i;
	for (i = 0; i < m_nJStar; i++)
	{
		m_P0[i] = GetDwordParam((string("p0_") + to_string(i)).data(), defaultP[i]) / 1000.0;
		m_M0[i] = GetDwordParam((string("m0_") + to_string(i)).data(), defaultM[i]) / 1000.0;
		m_MU0[i] = GetDwordParam((string("mu0_") + to_string(i)).data(), defaultMu[i]) / 1000.0;
		m_K3[i] = GetDwordParam((string("K3_") + to_string(i)).data(), defaultK3[i]) / 1000.0;
	}

	for (; i < sizeof(m_P0)/sizeof(m_P0[0]); i++)
	{
		m_P0[i] = 0.0;
		m_M0[i] = 0.0;
		m_MU0[i] = 0.0;
		m_K3[i] = 0.0;
	}

	m_bPlotBVD = GetDwordParam("PlotBVD", 0);
	m_bShowPScores = GetDwordParam("ShowPScores", 0);

	GetStringParam("PlotLevel", m_szPlotLevel, sizeof(m_szPlotLevel), "");
}


CParameters::~CParameters()
{
	::RegCloseKey(m_regKey);
}

void CParameters::SaveParams()
{
	PutStringParam("File", m_szPath);

	PutDwordParam("sgOrder", m_nSGOrder);			// 5 for raw data
	PutDwordParam("sgWindow", m_nSGWindow);		// 50 for raw data
	PutDwordParam("lambda0", m_nLambda0);
	PutDwordParam("lambda1", m_nLambda1);
	PutDwordParam("alpha", m_nAlpha);
	PutDwordParam("beta", m_nBeta);
	PutDwordParam("gamma", m_nGamma);
	PutDwordParam("nu", m_nNu);
	PutDwordParam("rho", (DWORD)(m_dRho * 1000));
	PutDwordParam("C1", m_nC1);
	PutDwordParam("C2", m_nC2);
	PutDwordParam("K1", (DWORD)m_dK1);
	PutDwordParam("K2", (DWORD)m_dK2);
	PutDwordParam("K4", (DWORD)(m_dK4 * 1000));
	PutDwordParam("epsilon", (DWORD)(m_dEpsilon * 1000000));
	PutDwordParam("eta1", (DWORD)(m_dEta1*1000.0));
	PutDwordParam("eta2", (DWORD)(m_dEta2 * 1000));
	PutDwordParam("Q1", (DWORD)(m_dQ1*1000.0));
	PutDwordParam("Q2", (DWORD)(m_dQ2*1000.0));
	PutDwordParam("Q2Star", (DWORD)(m_dQ2Star*1000.0));
	PutDwordParam("Q3", (DWORD)(m_dQ3*1000.0));
	PutDwordParam("K2", (DWORD)m_dK2);
	PutDwordParam("jMax", m_nJmax);

	PutDwordParam("nParams", m_nJStar);

	for (int i = 0; i < m_nJStar; i++)
	{
		PutDwordParam((string("p0_") + to_string(i)).data(), (DWORD)(m_P0[i] * 1000.0));
		PutDwordParam((string("m0_") + to_string(i)).data(), (DWORD)(m_M0[i] * 1000.0));
		PutDwordParam((string("mu0_") + to_string(i)).data(), (DWORD)(m_MU0[i] * 1000.0));
		PutDwordParam((string("K3_") + to_string(i)).data(), (DWORD)(m_K3[i] * 1000.0));
	}

	PutDwordParam("PlotBVD", m_bPlotBVD);
	PutDwordParam("ShowPScores", m_bShowPScores);
	PutStringParam("PlotLevel", m_szPlotLevel);
}

void CParameters::Serialize(CArchive & ar)
{
	if (ar.IsStoring())
	{
		ar << "Parameters";

		ar << m_nSGOrder;
		ar << m_nSGWindow;
		ar << m_nLambda0;
		ar << m_nLambda1;
		ar << m_nAlpha;
		ar << m_nBeta;
		ar << m_nGamma;
		ar << m_nNu;
		ar << m_dRho;
		ar << m_nJmax;
		ar << m_nC1;
		ar << m_nC2;
		ar << m_dK1;
		ar << m_dK2;
		ar << m_dK4;
		ar << m_dEpsilon;
		ar << m_dEta1;
		ar << m_dEta2;
		ar << m_dQ1;
		ar << m_dQ2;
		ar << m_dQ2Star;
		ar << m_dQ3;
		ar << m_nJStar;

		for (double var : m_P0) ar << var;
		for (double var : m_M0) ar << var;
		for (double var : m_MU0) ar << var;
		for (double var : m_K3) ar << var;
	}
	else
	{
		CheckArchiveLabel(ar, "Parameters");

		ar >> m_nSGOrder;
		ar >> m_nSGWindow;
		ar >> m_nLambda0;
		ar >> m_nLambda1;
		ar >> m_nAlpha;
		ar >> m_nBeta;
		ar >> m_nGamma;
		ar >> m_nNu;
		ar >> m_dRho;
		ar >> m_nJmax;
		ar >> m_nC1;
		ar >> m_nC2;
		ar >> m_dK1;
		ar >> m_dK2;
		ar >> m_dK4;
		ar >> m_dEpsilon;
		ar >> m_dEta1;
		ar >> m_dEta2;
		ar >> m_dQ1;
		ar >> m_dQ2;
		ar >> m_dQ2Star;
		ar >> m_dQ3;
		ar >> m_nJStar;

		for (double var : m_P0) ar >> var;
		for (double var : m_M0) ar >> var;
		for (double var : m_MU0) ar >> var;
		for (double var : m_K3) ar >> var;
	}
}
