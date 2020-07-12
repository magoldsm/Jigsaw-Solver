#include "pch.h"
#include "CPiece.h"
#include "BivertexArcDecomposition.h"
#include "Utilities.h"

using namespace Eigen;
using namespace std;
using namespace cv;

void BivertexArcDecomposition(CPiece& piece, double delta0, double delta1)
{
	Matrix<long, Dynamic, 2> BAIndices;
	BATable& BA = piece.m_Arcs;
	VectorXi& Pt2Arc = piece.m_Pt2Arc;


	int n1 = (int) piece.m_Contour.rows();
	int sizeBAI = n1 / 2;
	BAIndices.resize(sizeBAI, NoChange);

	const Curve& signature = piece.m_Signature;
	const Curve& contour = piece.m_Contour;
	const VectorXd& kappa = signature.col(0);
	const VectorXd& kappas = signature.col(1);
	auto xxx = signature.col(0);

	// Determine bivertex arc endpoint indices based on delta0 cut-off

#define HOFF_BVD
#ifndef HOFF_BVD
	VectorXd AbsKsMinusDelta = signature.y.array().abs() - delta0;
	const Mat mkappa(Size(1, n1), CV_64FC1, (void*) signature.x.data());
	const Mat mkappas(Size(1, n1), CV_64FC1, (void*)signature.y.data());
	Mat mAbsKsMinusDelta(Size(1, n1), CV_64FC1, (void*)AbsKsMinusDelta.data());
	Mat Levels(1, n1, CV_8UC1);
	Levels = mAbsKsMinusDelta > 0;
#else

	int shifter = 1;
	int flipper = 0;
	int nA = 0;
	int toggler = -1;

	double q = kappas[0];

	if (fabs(kappas[0]) < delta0 || kappas[0] == delta0)
		shifter = 0;

	for (int c1 = 0; c1 < n1; c1++)
	{
		q = kappas[c1];
		double r = kappas[(c1 + 1) % n1];
		int x = (c1 + 1) % n1;
		double s = kappas[x];

		if ((abs(kappas[c1]) - delta0)*(abs(kappas[(c1 + 1) % n1]) - delta0) <= 0)
		{
			//cout << "a  " << c1 << " | " << nA << " | " << toggler << " | " << shifter << " | " << flipper << " | " << ((nA + shifter /*+ 1 */ + flipper) & 1) << endl;
			BAIndices(nA, (nA + shifter /*+ 1 */ + flipper) & 1) = c1;
			toggler = (nA + shifter + flipper/* + 1*/) & 1;
			nA = nA + toggler;
			flipper = flipper + (toggler + 1) & 1;
		}
		else if ((abs(kappas[c1]) > delta0 &&
			abs(kappas[(c1 + 1) % n1]) > delta0 &&
			(kappas[c1] * kappas[(c1 + 1) % n1]) < 0)
			|| ((abs(kappas[c1]) - abs(kappas[(c1 - 1 + n1) % n1]) < 0 && abs(kappas[c1]) - abs(kappas[c1]) > 0)))
		{
			//cout << "b  " << c1 << " | " << nA << " | " << toggler << " | " << shifter << " | " << flipper << " | " << ((nA + shifter /*+ 1 */ + flipper) & 1) << endl;
			BAIndices(nA, (nA + shifter /*+ 1 */ + flipper) & 1) = c1;
			toggler = (nA + shifter + flipper/* + 1*/) & 1;
			nA = nA + toggler;
			flipper = flipper + (toggler + 1) & 1;
			//cout << "c  " << c1 << " | " << nA << " | " << toggler << " | " << shifter << " | " << flipper << " | " << ((nA + shifter /*+ 1 */ + flipper) & 1) << endl;
			BAIndices(nA, (nA + shifter +/* 1 +*/ flipper) & 1) = c1;
			toggler = (nA + shifter + flipper/* + 1*/) & 1;
			nA = nA + toggler;
			flipper = flipper + (toggler + 1) & 1;
		}
	}
	if (shifter == 1)
		BAIndices(0, 0) = BAIndices(nA, 0);
#endif

	//cout << BAIndices.topRows(nA) << endl;
	//cout << "----------------------------------------------------------------------" << endl;

	// Trim repeated arc
	BAIndices.bottomRows(sizeBAI - nA).setZero();
//	nA = nA - 1;


	// Reject arcs not meeting delta1 cut - off
	int c1 = 1;
	while (c1 < nA)
	{
		if (max(abs(kappa(BAIndices(c1, 1))), abs(kappa(BAIndices(c1, 0)))) < delta1)
		{
//			BAIndices.row(c1).setZero();
			if ((c1 + 1) < nA)
				//BAIndices.row(c1) = BAIndices.row(c1 + 1);
				BAIndices.block(c1, 0, nA - (c1 + 1), 2) = BAIndices.block(c1 + 1, 0, nA - (c1 + 1), 2);
			nA--;
			c1--;
		}
		c1++;
	}

//	cout << BAIndices.topRows(nA) << endl;

	// Assign ouput

	BA.resize(nA);
	Pt2Arc.resize(n1);
	Pt2Arc.setConstant(-1);

	long s = BAIndices(0, 0);
	long e = BAIndices(0, 1);
	long nTail = (int) contour.rows() - s;

	if (s > e)
	{
		BA(0).m_Contour.resize(nTail + e, 2);
		BA(0).m_Signature.resize(nTail + e, 2);
		BA(0).m_Contour.col(0) << contour.col(0).tail(nTail), contour.col(0).head(e);
		BA(0).m_Contour.col(1) << contour.col(1).tail(nTail), contour.col(1).head(e);
		BA(0).m_Signature.col(0) << signature.col(0).tail(nTail), signature.col(0).head(e);
		BA(0).m_Signature.col(1) << signature.col(1).tail(nTail), signature.col(1).head(e);
		Pt2Arc.tail(nTail).setZero();
		Pt2Arc.head(e).setZero();
	}
	else
	{
		BA(0).m_Contour.resize((e - s) + 1, 2);
		BA(0).m_Signature.resize((e - s) + 1, 2);
		BA(0).m_Contour.col(0) = contour.col(0).segment(s, (e - s) + 1);
		BA(0).m_Contour.col(1) = contour.col(1).segment(s, (e - s) + 1);
		BA(0).m_Signature.col(0) = signature.col(0).segment(s, (e - s) + 1);
		BA(0).m_Signature.col(1) = signature.col(1).segment(s, (e - s) + 1);
		Pt2Arc.segment(s, (e - s) + 1).setZero();
	}

	for (int c1 = 1; c1 < nA; c1++)
	{
		s = BAIndices(c1, 0);
		e = BAIndices(c1, 1);

		BA(c1).m_Contour.resize((e - s) + 1, 2);
		BA(c1).m_Signature.resize((e - s) + 1, 2);
		BA(c1).m_Contour.col(0) = contour.col(0).segment(s, (e - s) + 1);
		BA(c1).m_Contour.col(1) = contour.col(1).segment(s, (e - s) + 1);
		BA(c1).m_Signature.col(0) = signature.col(0).segment(s, (e - s) + 1);
		BA(c1).m_Signature.col(1) = signature.col(1).segment(s, (e - s) + 1);
		Pt2Arc.segment(s, (e - s) + 1).setConstant(c1);
	}
}

// Plot the dcomposition, showing each arc in alternating colors

void
PlotDecomposition(CPiece& piece, int iPiece)
{
	BATable& BA = piece.m_Arcs;
	VectorXi& Pt2Arc = piece.m_Pt2Arc;

	const int PlotSize = 900;

	Mat drawing = Mat::zeros(PlotSize, PlotSize, CV_8UC3);
	int sz = (int) BA.rows();
	Curve& contour = piece.m_Contour;

	// Determine the extents of all the arcs
	
	double xMax = contour.col(0).maxCoeff(), xMin = contour.col(0).minCoeff(), yMax = contour.col(1).maxCoeff(), yMin = contour.col(1).minCoeff();

	double xDiff = xMax - xMin;
	double yDiff = yMax - yMin;

	int pcsz = (int) piece.m_Contour.rows();

	for (int i = 0; i < pcsz; i++)
	{
		int X = (int)((PlotSize-1) * (piece.m_Contour(i, 0) - xMin) / xDiff);
		int Y = (int)((PlotSize-1) * (piece.m_Contour(i, 1) - yMin) / yDiff);

		if (X < 0) X = 0;
		if (Y < 0) Y = 0;
		drawing.at<Vec3b>(X, Y) = Vec3b(255, 255, 255);
		imshow("Decomposition", drawing);
	//waitKey(1);
	}

	for (int i = 0; i < sz; i++)
	{
		double Max = BA(i).m_Contour.col(0).maxCoeff();
		double Min = BA(i).m_Contour.col(0).minCoeff();

		xMax = max(xMax, Max);
		xMin = min(xMin, Min);

		Max = BA(i).m_Contour.col(1).maxCoeff();
		Min = BA(i).m_Contour.col(1).minCoeff();

		yMax = max(yMax, Max);
		yMin = min(yMin, Min);
	}

	xDiff = xMax - xMin;
	yDiff = yMax - yMin;

	unsigned char redChan = 255;
	unsigned char greenChan = 0;

	for (int i = 0; i < sz; i++)
	{
		Curve& contour = BA(i).m_Contour;
		int ctsz = (int) contour.rows();

		for (int j = 0; j < ctsz-1; j++)
		{
			int X1 = (int)((PlotSize - 1) * (contour(j, 0) - xMin) / xDiff);
			int Y1 = (int)((PlotSize - 1) * (contour(j, 1) - yMin) / yDiff);
			int X2 = (int)((PlotSize - 1) * (contour((j + 1) % ctsz, 0) - xMin) / xDiff);
			int Y2 = (int)((PlotSize - 1) * (contour((j + 1) % ctsz, 1) - yMin) / yDiff);

			if (X1 < 0) X1 = 0;
			if (Y1 < 0) Y1 = 0;
			if (X2 < 0) X2 = 0;
			if (Y2 < 0) Y2 = 0;
			line(drawing, Point(Y1, X1), Point(Y2, X2), Scalar(0, greenChan, redChan), 3);
		}

		redChan ^= 255;
		greenChan ^= 255;

		int X = (int)((PlotSize - 1) * (contour(0, 0) - xMin) / xDiff);
		int Y = (int)((PlotSize - 1) * (contour(0, 1) - yMin) / yDiff);

		char buff[1000];
		sprintf_s(buff, sizeof(buff), "(%d  %d)", i, (int) contour.rows());
		cv::putText(drawing, buff, Point(Y, X), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255), 2);
	}

	char buff[100];
	sprintf_s(buff, 100, "Decomposition %d", iPiece);
	imshow(buff, drawing);
}

// Plot the dcomposition, showing each arc in alternating colors

void
PlotKappasDecomposition(CPiece& piece, double delta0, int iPiece)
{
	BATable& BA = piece.m_Arcs;
	VectorXi& Pt2Arc = piece.m_Pt2Arc;

	const int PlotSize = 900;

	VectorXd norm = piece.m_Signature.col(1);
	int sz = (int)norm.rows();

	double min, max;

	MyNormv(norm, min, max);

	Mat drawing = Mat::zeros(Size(PlotSize, PlotSize), CV_8UC3);

	Vec3b colors[2] = { {0, 0, 255}, {0, 255, 0} };

	for (int i = 0; i < sz; i++)
	{
		int arc = Pt2Arc(i);

		int x = PlotSize * i / sz;
		int y = (PlotSize - 1) - (int)(norm(i) * (PlotSize - 1));
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		drawing.at<Vec3b>(y, x) = arc == -1 ? Vec3b(255, 255, 255) : colors[arc & 1];
	}

	if (delta0 > 0.0)
	{
		double dDeltaPlus = (delta0 - min) / (max - min);
		double dDeltaMinus = (-delta0 - min) / (max - min);
		line(drawing, Point(0, (int)(dDeltaPlus * PlotSize)), Point((PlotSize - 1), (int)(dDeltaPlus * PlotSize)), Scalar(255, 255, 255), 1);
		line(drawing, Point(0, (int)(dDeltaMinus * PlotSize)), Point((PlotSize - 1), (int)(dDeltaMinus * PlotSize)), Scalar(255, 255, 255), 1);
	}


	char buff[100];
	sprintf_s(buff, 100, "kappas decomp %d", iPiece);
	imshow(buff, drawing);
}