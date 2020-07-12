// Jigsaw Solver.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

#include "Savitsy-Golay.h"
#include "CParameters.h"
#include "Jigsaw Solver.h"
#include "Euclidean Signature.h"
#include "BivertexArcDecomposition.h"
#include "Utilities.h"
#include "SignatureSimilarity.h"
#include "CProgress.h"


using namespace std;
using namespace Eigen;
using namespace cv;

std::vector<CPiece> Pieces;
Eigen::VectorXd Weights;
VectorXd smoothVec, d1Vec, d2Vec;
double Dx, Dy, Dkappa, Dkappas;
double AverageLength;
long AverageSize;
CProgress Progress(4);

static CParameters p;					// Non-GUI - declare it here.
CParameters* pParams = &p;				// The one and only!


void CPScore::Display(double dP0)
{
	cout << "    ";
	for (size_t i = 0; i < m_Size; i++)
	{
		cout << setw(5) << i << " ";
	}
	cout << endl;
	for (size_t i = 0; i < m_Size; i++)
	{
		cout << setw(2) << i << "  ";
		for (size_t j = 0; j < m_Size; j++)
		{
			MatrixXd& arcscore = (*this)(i, j);
			if (arcscore.cols() == 0 && arcscore.rows() == 0)
				cout << "   [] ";
			else
			{
				char buff[100];
				sprintf_s(buff, 100, "%dx%d ", (int)arcscore.rows(), (int)arcscore.cols());
				cout << setw(6) << buff;
			}
		}
		cout << endl;
	}
}

void CPScore::Display(size_t nRow, size_t nCol, double dP0)
{
	MatrixXd& arcscore = (*this)(nRow, nCol);
	for (int i = 0; i < arcscore.rows(); i++)
	{
		for (int j = 0; j < arcscore.cols(); j++)
		{
			cout << setprecision(4) << setw(8) << arcscore(i, j);
		}
		cout << endl;
	}
}


void CProgress::UpdateReport()
{
	// __debugbreak();
}

void CProgress::Erase()
{
	// __debugbreak();
}

LRESULT CProgress::Plot(const Curve& curve, COLORREF color, int width)
{
	// __debugbreak();
	return 0;
}

void CProgress::Delete(LRESULT item)
{
	// __debugbreak();
}


int main()
{
	Progress(3).MakeSubbars(2);

	LARGE_INTEGER liStart, liFrequency, liSG, liEuclid, liBVD, liTotal;

	QueryPerformanceFrequency(&liFrequency);

	bool bSuccess = ReadPuzzle(Pieces, "Hoff-2 pieces.csv");
	int nPieces = (int) Pieces.size();

	QueryPerformanceCounter(&liStart);

	GenerateSGVector(0, pParams->m_nSGOrder, pParams->m_nSGWindow, smoothVec);
	GenerateSGVector(1, pParams->m_nSGOrder, pParams->m_nSGWindow, d1Vec);
	GenerateSGVector(2, pParams->m_nSGOrder, pParams->m_nSGWindow, d2Vec);

	QueryPerformanceCounter(&liSG);
	cout << "Savitsky-Golay " << (liSG.QuadPart - liStart.QuadPart) / (1.0*liFrequency.QuadPart) << " seconds" << endl;

	AverageSize = 0;
	AverageLength = 0.0;
	Weights.resize(nPieces);
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

		Weights[i] = Pieces[i].m_Signature.col(0).array().abs().sum();

		AverageSize += (long)Pieces[i].m_Contour.size();
		AverageLength += ((c.col(0) - cm1.col(0)).array().square() + (c.col(1) - cm1.col(1)).array().square()).sqrt().sum();
	}
		
	QueryPerformanceCounter(&liEuclid);
	cout << "Eucliean Sigs " << (liEuclid.QuadPart - liSG.QuadPart) / (1.0*liFrequency.QuadPart) << " seconds" << endl;

	AverageSize /= (long) Pieces.size();
	AverageLength /= Pieces.size();


	// Determine the characteristic size of the puzzle piece.
	// Basically, find a bounding rectangle that covers all piece contours and one that covers all their signatures

	MaxD(Pieces, &CPiece::m_Contour, Dx, Dy);
	MaxD(Pieces, &CPiece::m_Signature, Dkappa, Dkappas);

	// We use these to determine delta0 and delta1.  These are the amount of "noise" we tolerate
	// as we're dividing the pieces into their Bivertex Decompositions

	double delta0 = Dkappas / pParams->m_nLambda0;
	double delta1 = Dkappa / pParams->m_nLambda1;

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

	Progress.RestartReport(PROGRESS_EUCLID, true);
	Progress(PROGRESS_EUCLID).m_Percent = 1.0;
	Progress.UpdateReport();
	Progress.RestartReport(PROGRESS_BIVERTEX, true);
	Progress(PROGRESS_BIVERTEX).m_Percent = 1.0;
	Progress.UpdateReport();

	//for (int i = 0; i < nPieces; i++)
	//{
	//	PlotDecomposition(Pieces[i], i);
	//	PlotKappasDecomposition(Pieces[i], delta0, i);
	//	waitKey(1);
	//}

	//waitKey(0);

	PlacePieces();
	QueryPerformanceCounter(&liTotal);
	cout << "Total Runtime " << (liTotal.QuadPart - liStart.QuadPart) / (1.0*liFrequency.QuadPart) << " seconds" << endl;
	waitKey(0);
}

