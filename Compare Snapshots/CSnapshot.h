#pragma once
#include "../Jigsaw Solver/CParameters.h"
#include "../Jigsaw Solver/SignatureSimilarity.h"
#include "../Jigsaw Solver/Utilities.h"
#include "../Jigsaw Solver/CPiece.h"


class CSnapshot
{
public:
	CSnapshot(CString strFilename)
		: m_strFilename(strFilename)
	{

	}

	void ReadIn();
	void DumpScores();
	void Compare(const CSnapshot& other) const;

private:
	CString		m_strFilename;

	double						m_AverageLength;
	long							m_AverageSize;
	double						m_Dx;
	double						m_Dy;
	double						m_Dkappa;
	double						m_Dkappas;
	
	CParameters					m_Params;
	std::vector<CPlacement>		m_Placements;
	std::vector<CTracker>		m_Tracker;
	CPScore						m_PScores;
	std::vector<CPiece>			m_Pieces;
	std::vector<CFit>			m_Fits;

};

