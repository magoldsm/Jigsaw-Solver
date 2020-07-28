#include "pch.h"
#include "CSnapshot.h"

double AverageLength;
long AverageSize;
double Dx, Dy, Dkappa, Dkappas;
CPScore PScores;

std::vector<CFit> Fits;

#ifdef OPT
CArchive& operator<<(CArchive& ar, TwoPieces& tp)
{
	ar << tp[0];
	ar << tp[1];
	return ar;
}


CArchive& operator>>(CArchive& ar, TwoPieces& tp)
{
	ar >> tp[0];
	ar >> tp[1];
	return ar;
}
#endif


void CSnapshot::DumpScores()
{
	for (CPlacement p : m_Placements)
	{
		AlwaysOutput("%3d  %f %f %f %f\n", p.m_nPiece, p.m_Score(0), p.m_Score(1), p.m_Score(2), p.m_Score(3), p.m_Score(4));
	}
}


void CSnapshot::ReadIn()
{
	CFile fileLeft(m_strFilename, CFile::modeRead | CFile::typeBinary);
	CArchive ar(&fileLeft, CArchive::load);

	ar >> m_AverageLength >> m_AverageSize >> m_Dx >> m_Dy >> m_Dkappa >> m_Dkappas;
	m_Params.Serialize(ar);

	size_t sz;
	ar >> sz;
	for (int i = 0; i < sz; i++)
	{
		CPlacement p(ar);
		m_Placements.push_back(p);
	}

	ar >> sz;
	m_Tracker.resize(sz);
	for (int i = 0; i < sz; i++)
		m_Tracker[i].Serialize(ar);

	ar >> sz;
	m_Pieces.resize(sz);
	for (int i = 0; i < sz; i++)
		m_Pieces[i].Serialize(ar);

	m_PScores.Serialize(ar);

	ar >> sz;
	m_Fits.resize(sz);
	for (int i = 0; i < sz; i++)
		m_Fits[i].Serialize(ar);

}


#define CHECKD(x) \
	if (x != other.x)											\
	{															\
		CString strDiff;											\
		strDiff.Format(_T("%s: %f %f"), _T(#x), x, other.x);		\
		::AfxMessageBox(strDiff, MB_OK);							\
	}														

#define CHECKI(x) \
	if (x != other.x)											\
	{															\
		CString strDiff;											\
		strDiff.Format(_T("%s: %d %d"), _T(#x), x, other.x);		\
		::AfxMessageBox(strDiff, MB_OK);							\
	}														


#define CHECKI64(x) \
	if (x != other.x)											\
	{															\
		CString strDiff;											\
		strDiff.Format(_T("%s: %ld %ld"), _T(#x), x, other.x);		\
		::AfxMessageBox(strDiff, MB_OK);							\
	}														


#define CHECKDi(i, x) \
	if (x != other.x)											\
	{															\
		CString strDiff;											\
		strDiff.Format(_T("%s[%d]: %f %f"), _T(#x), i, x, other.x);		\
		::AfxMessageBox(strDiff, MB_OK);							\
	}														

#define CHECKIi(i, x) \
	if (x != other.x)											\
	{															\
		CString strDiff;											\
		strDiff.Format(_T("%s[%d]: %d %d"), _T(#x), i, x, other.x);		\
		::AfxMessageBox(strDiff, MB_OK);							\
	}														

#define CHECKDij(i, j, x) \
	if (x != other.x)											\
	{															\
		CString strDiff;											\
		strDiff.Format(_T("%s[%d,%d]: %f %f"), _T(#x), i, j, x, other.x);		\
		::AfxMessageBox(strDiff, MB_OK);							\
	}														

#define CHECKIij(i, j, x) \
	if (x != other.x)											\
	{															\
		CString strDiff;											\
		strDiff.Format(_T("%s[%d,%d]: %d %d"), _T(#x), i, j, x, other.x);		\
		::AfxMessageBox(strDiff, MB_OK);							\
	}														

#define CHECKDijk(i, j, k, x) \
	if (x != other.x)											\
	{															\
		CString strDiff;											\
		strDiff.Format(_T("%s[%d,%d,%d]: %f %f"), _T(#x), i, j, k, x, other.x);		\
		::AfxMessageBox(strDiff, MB_OK);							\
	}														

#define CHECKIijk(i, j, k, x) \
	if (x != other.x)											\
	{															\
		CString strDiff;											\
		strDiff.Format(_T("%s[%d,%d,%d]: %d %d"), _T(#x), i, j, k, x, other.x);		\
		::AfxMessageBox(strDiff, MB_OK);							\
	}														

#define CHECKIvi(i, x)									\
	{													\
		size_t min = std::min(x.size(), other.x.size());	\
		CHECKIi(i, x.size());							\
		for (int j = 0; j <min; j++)						\
		{												\
			CHECKIij(i, j, x[j]);						\
		}												\
	}

#define CHECKIvvi(i, x)												\
	{																\
		size_t min = std::min(x.size(), other.x.size());				\
		CHECKIi(i, x.size());										\
		for (int j = 0; j <min; j++)									\
		{															\
			size_t min = std::min(x[j].size(), other.x[j].size());	\
			CHECKIij(i, j, x[j]);									\
			for (int k = 0; k < min; k++)							\
			{														\
				CHECKIijk(i, j, k, x[j][k]);							\
			}														\
		}															\
	}

#define CHECKCURVEi(i, x)											\
	{																\
		size_t min = std::min(x.rows(), other.x.rows());				\
		CHECKIi(i, x.size());										\
		for (int j = 0; j < min; j++)								\
		{															\
			CHECKDij(i, j, x(j,0));									\
			CHECKDij(i, j, x(j,1));									\
		}															\
    }

#define CHECKCURVEij(i, j, x)										\
	{																\
		size_t min = std::min(x.rows(), other.x.rows());				\
		CHECKIij(i, j, x.size());									\
		for (int k = 0; k < min; k++)								\
		{															\
			CHECKDijk(i, j, k, x(k,0));								\
			CHECKDijk(i, j, k, x(k,1));								\
		}															\
    }

void CSnapshot::Compare(CSnapshot& other) 
{
	// Scalars

	CHECKD(m_AverageLength);
	CHECKI(m_AverageSize);
	CHECKD(m_Dx);
	CHECKD(m_Dy);
	CHECKD(m_Dkappa);
	CHECKD(m_Dkappas);

	// Parameters

	CHECKI(m_Params.m_nSGOrder);
	CHECKI(m_Params.m_nSGWindow);
	CHECKI(m_Params.m_nLambda0);
	CHECKI(m_Params.m_nLambda1);
	CHECKI(m_Params.m_nAlpha);
	CHECKI(m_Params.m_nBeta);
	CHECKI(m_Params.m_nGamma);
	CHECKI(m_Params.m_nNu);
	CHECKD(m_Params.m_dRho);
	CHECKI(m_Params.m_nJmax);
	CHECKI(m_Params.m_nC1);
	CHECKI(m_Params.m_nC2);
	CHECKD(m_Params.m_dK1);
	CHECKD(m_Params.m_dK2);
	CHECKD(m_Params.m_dK4);
	CHECKD(m_Params.m_dEpsilon);
	CHECKD(m_Params.m_dEta1);
	CHECKD(m_Params.m_dEta2);
	CHECKD(m_Params.m_dQ1);
	CHECKD(m_Params.m_dQ2);
	CHECKD(m_Params.m_dQ2Star);
	CHECKD(m_Params.m_dQ3);
	CHECKI(m_Params.m_nJStar);

	for (int i = 0; i < m_Params.m_nJStar; i++)
	{
		CHECKDi(i, m_Params.m_P0[i]);
		CHECKDi(i, m_Params.m_M0[i]);
		CHECKDi(i, m_Params.m_MU0[i]);
		CHECKDi(i, m_Params.m_K3[i]);
	}


	// Pieces

	CHECKI64(m_Pieces.size());

	for (int i = 0; i < m_Pieces.size(); i++)
	{

	//	CHECKIi(i, m_Pieces[i].m_Contour.size());					\
	//	for (int j = 0; j < m_Pieces[i].m_Contour.rows(); j++)		\
	//	{										\
	//		CHECKDij(i, j, m_Pieces[i].m_Contour(j,0));				\
	//		CHECKDij(i, j, m_Pieces[i].m_Contour(j,1));				\
	//	}

		CHECKCURVEi(i, m_Pieces[i].m_Contour);
		CHECKCURVEi(i, m_Pieces[i].m_Signature);
		CHECKDi(i, m_Pieces[i].m_Weight);
		CHECKIi(i, m_Pieces[i].m_nActive);

		CHECKIi(i, m_Pieces[i].m_Arcs.rows());
		for (int j = 0; j < m_Pieces[i].m_Arcs.rows(); j++)
		{
			CHECKCURVEij(i, j, m_Pieces[i].m_Arcs[j].m_Contour);
			CHECKCURVEij(i, j, m_Pieces[i].m_Arcs[j].m_Signature);
		}

		CHECKIvi(i, m_Pieces[i].m_Pt2Arc);
	}

	// Placements

	size_t plSz = m_Placements.size();
	CHECKI(m_Placements.size());

	for (int i = 0; i < plSz; i++)
	{
		CHECKIi(i, m_Placements[i].m_nPiece);
		CHECKDi(i, m_Placements[i].m_Score[0]);
		CHECKDi(i, m_Placements[i].m_Score[1]);
		CHECKDi(i, m_Placements[i].m_Score[2]);
		CHECKDi(i, m_Placements[i].m_Score[3]);
		CHECKDi(i, m_Placements[i].m_Score[4]);
		CHECKDi(i, m_Placements[i].m_gLock.theta);
		CHECKDi(i, m_Placements[i].m_gLock.dx);
		CHECKDi(i, m_Placements[i].m_gLock.dy);

		CHECKIi(i, m_Placements[i].m_Fit.m_Pieces[0]);
		CHECKIi(i, m_Placements[i].m_Fit.m_Pieces[1]);
		CHECKIi(i, m_Placements[i].m_Fit.m_Size);

		for (int j = 0; j < m_Placements[i].m_Fit.m_Size; j++)
		{
			CHECKIij(i, j, m_Placements[i].m_Fit.m_Arcs(j, 0));
			CHECKIij(i, j, m_Placements[i].m_Fit.m_Arcs(j, 1));
			CHECKDij(i, j, m_Placements[i].m_Fit.m_ArcTrans[j].theta);
			CHECKDij(i, j, m_Placements[i].m_Fit.m_ArcTrans[j].dx);
			CHECKDij(i, j, m_Placements[i].m_Fit.m_ArcTrans[j].dy);
		}

		CHECKDi(i, m_Placements[i].m_Fit.m_gFit.theta);
		CHECKDi(i, m_Placements[i].m_Fit.m_gFit.dx);
		CHECKDi(i, m_Placements[i].m_Fit.m_gFit.dy);
		CHECKDi(i, m_Placements[i].m_Fit.m_Score);
		CHECKIi(i, m_Placements[i].m_Fit.m_Slot);

		CHECKIi(i, m_Placements[i].m_Neighbors.size());
		for (int j = 0; j < m_Placements[i].m_Neighbors.size(); j++)
		{
			CHECKIi(i, m_Placements[i].m_Neighbors[j]);
		}
	}

	// Tracker

	CHECKI(m_Tracker.size());
	for (int i = 0; i < m_Tracker.size(); i++)
	{
		CHECKIvi(i, m_Tracker[i].m_PlacedPieces);
		CHECKIvi(i, m_Tracker[i].m_RemainingPieces);
		CHECKIvvi(i, m_Tracker[i].m_InactiveArcs);
		CHECKIvvi(i, m_Tracker[i].m_ActiveArcs);
		CHECKIvvi(i, m_Tracker[i].m_InactivePoints);
		CHECKIvvi(i, m_Tracker[i].m_ActivePoints);
		CHECKIvi(i, m_Tracker[i].m_Pc2Place);
		CHECKCURVEi(i, m_Tracker[i].m_SolvedPuzzleBoundary);
		
		CHECKIi(i, m_Tracker[i].m_SPB_Pt2PcPt.rows());
		for (int j = 0; j < m_Tracker[i].m_SPB_Pt2PcPt.rows(); j++)
		{
			CHECKIij(i, j, m_Tracker[i].m_SPB_Pt2PcPt(j, 0));
			CHECKIij(i, j, m_Tracker[i].m_SPB_Pt2PcPt(j, 1));
		}

		CHECKIvi(i, m_Tracker[i].m_SP_Bdry_PtIcs_3);
		CHECKIvi(i, m_Tracker[i].m_SP_Bdry_PtIcs);
	}


	// Fits

	CHECKI(m_Fits.size());
	for (int i = 0; i < m_Fits.size(); i++)
	{
		CHECKIi(i, m_Fits[i].m_Pieces[0]);
		CHECKIi(i, m_Fits[i].m_Pieces[1]);
		CHECKIi(i, m_Fits[i].m_Size);

		for (int j = 0; j < m_Fits[i].m_Size; j++)
		{
			CHECKIij(i, j, m_Fits[i].m_Arcs(j, 0));
			CHECKIij(i, j, m_Fits[i].m_Arcs(j, 1));
		}

		CHECKIi(i, m_Fits[i].m_ArcTrans.size());
		for (int j = 0; j < m_Fits[i].m_ArcTrans.size(); j++)
		{
			CHECKDij(i, j, m_Fits[i].m_ArcTrans[j].theta);
			CHECKDij(i, j, m_Fits[i].m_ArcTrans[j].dx);
			CHECKDij(i, j, m_Fits[i].m_ArcTrans[j].dy);
		}
		CHECKDi(i, m_Fits[i].m_gFit.theta);
		CHECKDi(i, m_Fits[i].m_gFit.dx);
		CHECKDi(i, m_Fits[i].m_gFit.dy);
		CHECKDi(i, m_Fits[i].m_Score);
		CHECKIi(i, m_Fits[i].m_Slot);
	}


}

