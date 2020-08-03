#include "pch.h"
#include "CPiece.h"
#include "SignatureSimilarity.h"
#include "CParameters.h"
#include "Utilities.h"
#include "Jigsaw Solver.h"
#include "Lock.h"
#include "CProgress.h"

using namespace Eigen;
using namespace std;

CPScore::CPScore(size_t sz) : m_Size(sz)
{
	m_ArcScores = new MatrixXd[sz * sz];
}


CPScore::~CPScore()
{
	if (m_ArcScores) delete[] m_ArcScores;
}

void CPScore::SetSize(size_t sz)
{
	if (m_ArcScores) delete[] m_ArcScores;
	m_Size = sz;
	m_ArcScores = new MatrixXd[sz * sz];
}



void CPScore::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << "PScores";

		ar << m_Size;
		for (int i = 0; i < m_Size * m_Size; i++)
			ar << m_ArcScores[i];
	}
	else
	{
		CheckArchiveLabel(ar, "PScores");

		ar >> m_Size;
		if (m_ArcScores) delete[] m_ArcScores;
		SetSize(m_Size);
		for (int i = 0; i < m_Size * m_Size; i++)
			ar >> m_ArcScores[i];
	}
}





void CFit::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << "CFit";

		if (m_Pieces[0] < -1 || m_Pieces[0] > 100 || m_Pieces[1] < -1 || m_Pieces[1] > 100)
		__debugbreak();

		ar << m_Pieces;
		ar << m_Size;
		ar << m_Arcs;
		ar << m_ArcTrans.size();
		for (GTransform& t : m_ArcTrans) t.Serialize(ar);
		//		ar << m_ArcTrans;
		m_gFit.Serialize(ar);
		ar << m_Score;
		ar << m_Slot;
	}
	else
	{
		CheckArchiveLabel(ar, "CFit");

		ar >> m_Pieces;
		ar >> m_Size;
		ar >> m_Arcs;
		//		ar >> m_ArcTrans;
		size_t sz;
		ar >> sz;
		m_ArcTrans.resize(sz);
		for (GTransform& t : m_ArcTrans) t.Serialize(ar);
		m_gFit.Serialize(ar);
		ar >> m_Score;
		ar >> m_Slot;
	}
}


void CFit::Dump(int nIdx, ofstream& file)
{
	file << "Index: " << nIdx << endl;
	file << "    Pieces: [" << m_Pieces[0] << " " << m_Pieces[1] << "]\n";
	file << "    " << m_Size << " Arcs\n";
	Eigen::MatrixXd pscore = PScores(m_Pieces[0], m_Pieces[1]);
	for (int i = 0; i < m_Size; i++)
	{
		file << "    " << setw(3) << m_Arcs(i, 0) << setw(3) << m_Arcs(i, 1);
		file << "  <" << pscore(m_Arcs(i, 0), m_Arcs(i, 1)) << ">";
		if (i < m_ArcTrans.size())
			file << "   (" << m_ArcTrans[i].theta << ", " << m_ArcTrans[i].dx << ", " << m_ArcTrans[i].dy << ")";
		file << endl;
	}
	file << "    gFit: (" << m_gFit.theta << ", " << m_gFit.dx << ", " << m_gFit.dy << ")\n";
	file << "    Score: " << m_Score << endl;
	file << "    Slot: " << m_Slot << endl;
	file << endl << "---------------------------------------------------------" << endl << endl;
}

void CFit::Dump(const char* pszFilename)
{
	char szBuff[MAX_PATH];

	for (int i = 1; ; i++)
	{
		sprintf_s(szBuff, sizeof(szBuff), "%s%04d.txt", pszFilename, i);
		struct stat statbuff;

		if (stat(szBuff, &statbuff) != 0)
			break;
	}

	ofstream file;
	file.open(szBuff, std::fstream::out | std::fstream::trunc);

	for (int i = 0; i < Fits.size(); i++)
		Fits[i].Dump(i, file);
}


void CPlacement::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << "Placement";
		ar << m_nPiece;
		ar << m_Score;
		m_gLock.Serialize(ar);
		m_Fit.Serialize(ar);
		ar << m_Neighbors;
	}
	else
	{
		CheckArchiveLabel(ar, "Placement");

		ar >> m_nPiece;
		ar >> m_Score;
		m_gLock.Serialize(ar);
		m_Fit.Serialize(ar);
		ar >> m_Neighbors;
	}
}

CPlacement::CPlacement(CArchive& ar)
{
	CheckArchiveLabel(ar, "Placement");

	ar >> m_nPiece;
	ar >> m_Score;
	m_gLock.Serialize(ar);
	m_Fit.Serialize(ar);
	ar >> m_Neighbors;
}



void GTransform::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << "GTransform";
		ar << theta << dx << dy;
	}
	else
	{
		CheckArchiveLabel(ar, "GTransform");

		ar >> theta >> dx >> dy;
	}
}

void CTracker::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << "Tracker";

		ar << m_PlacedPieces;
		ar << m_RemainingPieces;
		ar << m_InactiveArcs;
		ar << m_ActiveArcs;
		ar << m_InactivePoints;
		ar << m_ActivePoints;
		ar << m_Pc2Place;
		ar << m_SolvedPuzzleBoundary;
		ar << m_SPB_Pt2PcPt;
		ar << m_SP_Bdry_PtIcs_3;
		ar << m_SP_Bdry_PtIcs;
	}
	else
	{
		CheckArchiveLabel(ar, "Tracker");

		ar >> m_PlacedPieces;
		ar >> m_RemainingPieces;
		ar >> m_InactiveArcs;
		ar >> m_ActiveArcs;
		ar >> m_InactivePoints;
		ar >> m_ActivePoints;
		ar >> m_Pc2Place;
		ar >> m_SolvedPuzzleBoundary;
		ar >> m_SPB_Pt2PcPt;
		ar >> m_SP_Bdry_PtIcs_3;
		ar >> m_SP_Bdry_PtIcs;
	}
}
