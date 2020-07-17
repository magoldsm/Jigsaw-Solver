#include "pch.h"
#include "CProgress.h"
#include "CParameters.h"
#include "Savitsy-Golay.h"
#include "Euclidean Signature.h"
#include "BivertexArcDecomposition.h"
#include "Utilities.h"
#include "SignatureSimilarity.h"
#include "CPiece.h"

using namespace std;
using namespace Eigen;
using namespace cv;

extern std::vector<CPiece> Pieces;
extern Eigen::VectorXd smoothVec, d1Vec, d2Vec;
extern double AverageLength;
extern long AverageSize;
extern double Dx, Dy, Dkappa, Dkappas;


LARGE_INTEGER liStart, liFrequency, liSG, liEuclid, liBVD, liTotal;


void Solver(const char* pszResume)
{
	double delta0 = 0;
	double delta1 = 0;

	bool bResume = pszResume != nullptr;

	QueryPerformanceFrequency(&liFrequency);

	bool bSuccess = ReadPuzzle(Pieces, pParams->m_szPath);
	int nPieces = (int)Pieces.size();

	QueryPerformanceCounter(&liStart);

	GenerateSGVector(0, pParams->m_nSGOrder, pParams->m_nSGWindow, smoothVec);
	GenerateSGVector(1, pParams->m_nSGOrder, pParams->m_nSGWindow, d1Vec);
	GenerateSGVector(2, pParams->m_nSGOrder, pParams->m_nSGWindow, d2Vec);

	QueryPerformanceCounter(&liSG);
	cout << "Savitsky-Golay " << (liSG.QuadPart - liStart.QuadPart) / (1.0*liFrequency.QuadPart) << " seconds" << endl;

	if (bResume)
	{
		CFile file(CString(pszResume), CFile::modeRead | CFile::typeBinary);
		CArchive ar(&file, CArchive::load);
		
		ar >> AverageLength >> AverageSize >> Dx >> Dy >> Dkappa >> Dkappas;
		pParams->Serialize(ar);

		size_t sz;
		ar >> sz;
		for (int i = 0; i < sz; i++)
		{
			CPlacement p(ar);
			Placements.push_back(p);
		}

		ar >> sz;
		Tracker.resize(sz);
		for (int i = 0; i < sz; i++)
			Tracker[i].Serialize(ar);

		ar >> sz;
		Pieces.resize(sz);
		for (int i = 0; i < sz; i++)
			Pieces[i].Serialize(ar);

		PScores.Serialize(ar);

		ar >> sz;
		Fits.resize(sz);
		for (int i = 0; i < sz; i++)
			Fits[i].Serialize(ar);

	}
	else
	{
		AverageSize = 0;
		AverageLength = 0.0;
		atomic<int> nDone(0);
		Progress.RestartReport(PROGRESS_EUCLID, true);

		FOR_START(i, 0, nPieces)
			CalcEuclideanSignature(Pieces[i], smoothVec, d1Vec, d2Vec);
		nDone += 1;
		Progress(PROGRESS_EUCLID).m_Percent = 1.0*nDone / nPieces;
		Progress.UpdateReport();
		FOR_END

		for (int i = 0; i < nPieces; i++)
		{
			Curve& c = Pieces[i].m_Contour;
			Curve cm1;
			cm1.resizeLike(c);

			circShift(c, cm1, -1);

			Pieces[i].m_Weight = Pieces[i].m_Signature.col(0).array().abs().sum();

			AverageSize += (long)Pieces[i].m_Contour.rows();
			AverageLength += ((c.col(0) - cm1.col(0)).array().square() + (c.col(1) - cm1.col(1)).array().square()).sqrt().sum();
		}

		QueryPerformanceCounter(&liEuclid);
		cout << "Eucliean Sigs " << (liEuclid.QuadPart - liSG.QuadPart) / (1.0*liFrequency.QuadPart) << " seconds" << endl;

		AverageSize /= (long)Pieces.size();
		AverageLength /= Pieces.size();


		// Determine the characteristic size of the puzzle piece.
		// Basically, find a bounding rectangle that covers all piece contours and one that covers all their signatures

		MaxD(Pieces, &CPiece::m_Contour, Dx, Dy);
		MaxD(Pieces, &CPiece::m_Signature, Dkappa, Dkappas);

		// We use these to determine delta0 and delta1.  These are the amount of "noise" we tolerate
		// as we're dividing the pieces into their Bivertex Decompositions

		delta0 = Dkappas / pParams->m_nLambda0;
		delta1 = Dkappa / pParams->m_nLambda1;

		nDone = 0;
		Progress.RestartReport(PROGRESS_BIVERTEX, true);

		FOR_START(i, 0, nPieces)
			BivertexArcDecomposition(Pieces[i], delta0, delta1/*, BA[i], Pt2Arc[i]*/);
		nDone++;
		Progress(PROGRESS_BIVERTEX).m_Percent = 1.0*nDone / nPieces;
		Progress.UpdateReport();
		FOR_END

			QueryPerformanceCounter(&liBVD);
		cout << "Bivertex Decomp " << (liBVD.QuadPart - liEuclid.QuadPart) / (1.0*liFrequency.QuadPart) << " seconds" << endl;
	}

	if (!bResume)
	{
		Progress.RestartReport(PROGRESS_EUCLID, true);
			Progress(PROGRESS_EUCLID).m_Percent = 1.0;
			Progress.UpdateReport();
			Progress.RestartReport(PROGRESS_BIVERTEX, true);
			Progress(PROGRESS_BIVERTEX).m_Percent = 1.0;
			Progress.UpdateReport();

			if (pParams->m_bPlotBVD)
			{
				for (int i = 0; i < nPieces; i++)
				{
					PlotDecomposition(Pieces[i], i);
					PlotSignatureDecomposition(Pieces[i], 0, delta1, i);
					PlotSignatureDecomposition(Pieces[i], 1, delta0, i);
					//	waitKey(1);
				}
			}

		waitKey(0);
	}

	PlacePieces();
	QueryPerformanceCounter(&liTotal);
	cout << "Total Runtime " << (liTotal.QuadPart - liStart.QuadPart) / (1.0*liFrequency.QuadPart) << " seconds" << endl;
//	waitKey(0);

}