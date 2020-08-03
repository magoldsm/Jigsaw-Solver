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
using namespace cv;

vector<CPlacement> Placements;
vector<CTracker> Tracker;
CPScore PScores;
vector<CFit> Fits;

vector<int> nFitSteps;

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


inline double
QuickPow(double x, int i)
{
	double xsq;

	switch (i)
	{
	case 0:
		return 1.0;
	case 1:
		return x;
	case 2:
		return x * x;
	case 3:
		return x * x * x;
	case 4:
		xsq = x * x;
		return xsq * xsq;
	case 5:
		xsq = x * x;
		return xsq * xsq * x;
	case 6:
		xsq = x * x;
		return xsq * xsq * xsq;
	default:
		return pow(x, i);
	}
}

// Equation numbers are from paper "Extensions of Invariant Signatures for Object Recognition

static double
_SignatureSimilarity(const Curve& sig1, const Curve& sig2, double D)
{
	int sz1 = (int) sig1.rows();
	int sz2 = (int) sig2.rows();

	VectorXd h = VectorXd::Zero(sz1);

	// Equations 3.4, 3.5

	// Calculate strengths of correspondence
	
	FOR_START(i, 0, sz1)
		Matrix<double, 1, 2> t = sig1.row(i);
		VectorXd d = (sig2.rowwise() - t).array().square().rowwise().sum().sqrt();
		//VectorXd d = ((sig2.col(0) - t.x).array().square() + (sig2.col(1)-t.y).array().square()).sqrt();

		// 	Calculate and sum h over j

		for (int j = 0; j < sz2; j++)
		{
			if (d[j] < D)
			{
				h[i] += 1.0 / (QuickPow((d[j] / (D - d[j])), pParams->m_nGamma) + pParams->m_dEpsilon);
			}
		}
	FOR_END

	// Equation 3.8

	VectorXd pPoint = h.array() / (h.array() + pParams->m_nC1);

	// Equation 3.10

	VectorXd kPow = sig1.col(0).array().pow(pParams->m_nAlpha);
	VectorXd Prod = (pPoint.array() * kPow.array());
	double num = Prod.sum();
	double den = kPow.sum();
	return num/den;

}


double
SignatureSimilarity(const Curve& sig1, const Curve& sig2, double D)
{
	double pHat1 = _SignatureSimilarity(sig1, sig2, D);
	double pHat2 = _SignatureSimilarity(sig2, sig1, D);

	return min(pHat1, pHat2);
}

/*
% This function applies a rigid motion to inputted points
%{
%--------------------------------------------------------------------
INPUTS
%--------------------------------------------------------------------

'points':   This should be n points represented as an n-by-2 matrix,
			each row of which specifies a point in R^2.

'trans':    This input should be a matrix of the form [theta a b]
			where these parameters specify a rigid motion as
			described in [2] (a translation by [a b] and a rotation
			by theta radians around the origin).

%--------------------------------------------------------------------

%--------------------------------------------------------------------
OUTPUTS
%--------------------------------------------------------------------

'result':   This output gives the transformed points as an n-by-2
			matrix, each row of which specifies a point in R^2.

%--------------------------------------------------------------------
*/

Curve
TransformCurve(const Curve& points, const GTransform& trans)
{
	Curve result;
	result.resizeLike(points);

	double c = cos(trans.theta);
	double s = sin(trans.theta);

	result.col(0) = (points.col(0)*c - points.col(1)*s).array() + trans.dx;
	result.col(1) = (points.col(0)*s + points.col(1)*c).array() + trans.dy;

	return result;
}

/*
	%--------------------------------------------------------------------
	 Compute the rigid motion angle, as described in
	 Sect. 3.3.of[2]
	%--------------------------------------------------------------------
	INPUTS
	%--------------------------------------------------------------------
	
	'curve1':   This should be a discretized planar curve of n points
				represented as an n - by - 2 matrix, each row of which
				specifies a point in R^2.
	
	'curve2' : This should be a discretized planar curve of m points
				represented as an m - by - 2 matrix, each row of which
				specifies a point in R^2.
	
	'signature1' : This should be a discrete approximation to the
				Euclidean signature of 'curve1', represented as an
				n - by - 2 matrix, each row of which specifies a point in
				R^2.
	
	'signature2' : This should be a discrete approximation to the
				Euclidean signature of 'curve2', represented as an
				m - by - 2 matrix, each row of which specifies a point in
				R^2.
	
	'beta' :	This real number specifies the power by which kappa_s
				weights an average as described in Sect. 3.3.
	
	%--------------------------------------------------------------------
	
	
	
	%--------------------------------------------------------------------
	OUTPUTS
	%--------------------------------------------------------------------
	
	'theta':    The resulting rigid motion angle, as described in
				Sect. 3.3.of[2].
	
	%--------------------------------------------------------------------
	%
*/

static double
Rigid_Motion_Angle(const Curve& curve1, const Curve& curve2, const Curve& signature1, const Curve& signature2, double beta)
{
	VectorXd dx1, dx2, dy1, dy2;
	dx1.resizeLike(curve1.col(0));
	dx2.resizeLike(curve2.col(0));
	dy1.resizeLike(curve1.col(1));
	dy2.resizeLike(curve2.col(1));

	int sz1 = (int)dx1.rows();
	int sz2 = (int)dx2.rows();

	dx1.block(1, 0, sz1 - 2, 1) = curve1.col(0).topRows(sz1 - 2) - curve1.col(0).bottomRows(sz1 - 2);
	dx2.block(1, 0, sz2 - 2, 1) = curve2.col(0).topRows(sz2 - 2) - curve2.col(0).bottomRows(sz2 - 2);
	dy1.block(1, 0, sz1 - 2, 1) = curve1.col(1).topRows(sz1 - 2) - curve1.col(1).bottomRows(sz1 - 2);
	dy2.block(1, 0, sz2 - 2, 1) = curve2.col(1).topRows(sz2 - 2) - curve2.col(1).bottomRows(sz2 - 2);

	dx1[0] = dx1[1]; dx1[sz1 - 1] = dx1[sz1 - 2];
	dx2[0] = dx2[1]; dx2[sz2 - 1] = dx2[sz2 - 2];
	dy1[0] = dy1[1]; dy1[sz1 - 1] = dy1[sz1 - 2];
	dy2[0] = dy2[1]; dy2[sz2 - 1] = dy2[sz2 - 2];

	//dx1.array() *= signature1.col(1).array();
	//dy1.array() *= signature1.col(1).array();

	//dx2.array() *= signature2.col(1).array();
	//dy2.array() *= signature2.col(1).array();

	VectorXd ks1pow = signature1.col(1).array().abs().pow(beta);
	VectorXd ks2pow = signature2.col(1).array().abs().pow(beta);
	//double ks1powsum = ks1pow.sum();
	//double ks2powsum = ks2pow.sum();

	// The original code divides the X and Y componenets by ksXpowsum.  I believe this
	// is unnecessary and I've removed it.

	double s1 = (dy1.array() * ks1pow.array()).sum()/* / ks1powsum*/;
	double c1 = (dx1.array() * ks1pow.array()).sum()/* / ks1powsum*/;
	double s2 = (dy2.array() * ks2pow.array()).sum()/* / ks2powsum*/;
	double c2 = (dx2.array() * ks2pow.array()).sum()/* / ks2powsum*/;

	double angle1 = atan2(s1, c1);
	double angle2 = atan2(s2, c2);

	return fmod(angle2 - angle1 + 2 * PI, 2 * PI);
}

/*
% This function reconstructs a rigid motion translation as described
% in Sect. 3.3 of [2].
%{
%--------------------------------------------------------------------
INPUTS
%--------------------------------------------------------------------

'curve1':   This should be a discretized planar curve of n points
			represented as an n-by-2 matrix, each row of which
			specifies a point in R^2.

'curve2':   This should be a discretized planar curve of m points
			represented as an m-by-2 matrix, each row of which
			specifies a point in R^2.

'signature1':   This should be a discrete approximation to the
			Euclidean signature of 'curve1', represented as an
			n-by-2 matrix, each row of which specifies a point in
			R^2.

'signature2':   This should be a discrete approximation to the
			Euclidean signature of 'curve2', represented as an
			m-by-2 matrix, each row of which specifies a point in
			R^2.

'theta':    This should be the angle of the rigid motion as
			outputted by Rigid_Motion_Angle().

'beta':     This real number specifies the power by which kappa_s
			weights an average as described in Sect. 3.3.

%--------------------------------------------------------------------


%--------------------------------------------------------------------
OUTPUTS
%--------------------------------------------------------------------

'G.dx':     The resulting rigid motion horizontal translation as
			described in Sect. 3.3 of [2].

'G.dy':     The resulting rigid motion vertical translation as
			described in Sect. 3.3 of [2].

%--------------------------------------------------------------------
%}
*/

static void
Rigid_Motion_Translation(GTransform& G, const Curve& curve1, const Curve& curve2, const Curve& signature1, const Curve& signature2, double theta, double beta)
{
	VectorXd ks1pow = signature1.col(1).array().pow(beta);
	VectorXd ks2pow = signature2.col(1).array().pow(beta);
	double ks1powsum = ks1pow.sum();
	double ks2powsum = ks2pow.sum();

	// Calculate center of mass weighted by kappa_s

	double cm1x = (curve1.col(0).array() * ks1pow.array()).sum() / ks1powsum;
	double cm1y = (curve1.col(1).array() * ks1pow.array()).sum() / ks1powsum;
	double cm2x = (curve2.col(0).array() * ks2pow.array()).sum() / ks2powsum;
	double cm2y = (curve2.col(1).array() * ks2pow.array()).sum() / ks2powsum;

	G.dx = cm2x - cm1x * cos(theta) + cm1y * sin(theta);
	G.dy = cm2y - cm1x * sin(theta) - cm1y * cos(theta);
}

/*
% This function calculates the mu score of a collection of rigid
% motions as described in [2].
%{
%--------------------------------------------------------------------
INPUTS
%--------------------------------------------------------------------

'trans':    This should be a n-by-3 matrix each row of which
			represents a transformation as [theta a b] where these
			parameters specify a rigid motion as described in [2].

'D_x':      The characteristic horizontal distance used in the mu
			score calculation as described in [2].

'D_y':      The characteristic vertical distance used in the mu score
			calculation as described in [2].

'C_2':      The parameter used in the mu score calculation as
			described in [2].

%--------------------------------------------------------------------


%--------------------------------------------------------------------
OUTPUTS
%--------------------------------------------------------------------

'mu':       This output gives resulting mu score as described in [2].

%--------------------------------------------------------------------
%}
*/

static GTransform range(vector<GTransform>& trans)
{
	GTransform result;
	double minX = MAXINT; double maxX = -MAXINT;
	double minY = MAXINT; double maxY = -MAXINT;

	for (int i = 0; i < trans.size(); i++)
	{
		GTransform& t = trans[i];
#ifndef NEW
		minX = min(minX, t.dx);
		minY = min(minY, t.dy);
		maxX = max(maxX, t.dx);
		maxY = max(maxY, t.dy);
#else
		minX = min(minX, t.dx);
		maxY = max(maxY, t.dy);
		maxY = max(maxY, t.dy);
#endif
	}

	result.dx = maxX - minX;
	result.dy = maxY - minY;

	return result;
}

double
muScore(vector<GTransform>& trans, double Dx, double Dy, double C2)
{
	// % Calculate differences between angles

	int sz = (int)trans.size();
	MatrixXd difAng = MatrixXd::Zero(sz, sz);

	for (int c1 = 0; c1 < sz; c1++)
		for (int c2 = c1; c2 < sz; c2++)
		{
			difAng(c1, c2) = fmod(trans[c1].theta - trans[c2].theta + 2 * PI, 2 * PI);
			if (difAng(c1, c2) > PI)
				difAng(c1, c2) = 2 * PI - difAng(c1, c2);
		}

	GTransform temp = range(trans);

	double mu = 1 - C2 * (difAng.maxCoeff() / PI + temp.dx / Dx + temp.dy / Dy);

	//if (mu < 0)
	//{
	//	AlwaysOutput("%f\n", mu);
	//	__debugbreak();
	//	::PostMessage(NULL, 0, 0, (WPARAM)mu);
	//}

	return mu < 0 ? 0 : mu;
}
/*

mu = 1-C_2*sum([max(difAng.maxCoeff())/pi  range(trans(:, 2:3))./[D_x  D_y]], 2);

*/

void CalcMeanTranslation(CFit& fit)
{
	int sz = (int)fit.m_ArcTrans.size();
	double tx = 0.0, ty = 0.0;
	for (int i = 0; i < sz; i++)
	{
		GTransform& trans = fit.m_ArcTrans[i];
		tx += trans.dx;
		ty += trans.dy;
	}
	fit.m_gFit.dx = tx / sz;
	fit.m_gFit.dy = ty / sz;

}


void
CalcAverageTheta(CFit& fit)
{
	double cang = 0.0, sang = 0.0;
	for (int c4 = 0; c4 < fit.m_Size; c4++)
	{
		cang += cos(fit.m_ArcTrans[c4].theta);
		sang += sin(fit.m_ArcTrans[c4].theta);
	}

	fit.m_gFit.theta = atan2(sang, cang);
	fit.m_gFit.dx = 0.0;
	fit.m_gFit.dy = 0.0;
}


void
CalcTranslation(CFit& fit, vector<CPlacement>& placements, vector<CTracker>& tracker)
{
	for (int c3 = 0; c3 < fit.m_Arcs.rows(); c3++)
	{
		const Curve& curve1 = Pieces[fit.m_Pieces[0]].m_Arcs(fit.m_Arcs(c3, 0)).m_Contour;
		Curve curve2 = TransformCurve(
			Pieces[fit.m_Pieces[1]].m_Arcs(fit.m_Arcs(c3, 1)).m_Contour,
			placements[tracker.back().m_Pc2Place[fit.m_Pieces[1]]].m_gLock);
		curve2.colwise().reverseInPlace();

		const Curve& sig1 = Pieces[fit.m_Pieces[0]].m_Arcs(fit.m_Arcs(c3, 0)).m_Signature;
		Curve sig2 = Orient_Reverse(Pieces[fit.m_Pieces[1]].m_Arcs(fit.m_Arcs(c3, 1)).m_Signature);

		Rigid_Motion_Translation(fit.m_ArcTrans[c3],
			curve1,
			curve2,
			sig1,
			sig2, fit.m_gFit.theta, pParams->m_nBeta);
	}
	fit.m_Score = muScore(fit.m_ArcTrans, Dx, Dy, pParams->m_nC2);
	CalcMeanTranslation(fit);
}

void PlotPlacement(const CPlacement& placement)
{
	Curve& c = TransformCurve(Pieces[placement.m_nPiece].m_Contour, placement.m_gLock);
	auto meanX = (long) c.col(0).mean();
	auto meanY = (long) c.col(1).mean();
	Progress.Plot(c);

	char szBuff[100];
	_itoa_s(placement.m_nPiece, szBuff, sizeof(szBuff), 10);

	Progress.Text(szBuff, meanX, meanY);
}

void ComputePScores(int nPiece1, int nPiece2, int j)
{
	//DebugOutput("PScore: %d %d\n", nPiece1, nPiece2);

	if (PScores.IsEmpty(nPiece1, nPiece2))
	{
		PScores(nPiece1, nPiece2) =
			MatrixXd::Zero(Pieces[nPiece1].m_Arcs.size(), Pieces[nPiece2].m_Arcs.size());

		// c4	counts over active arcs in Piece1
		// c5	counts over active arcs in Piece2

		for (int c4 = 0; c4 < Tracker.back().m_ActiveArcs[nPiece1].size(); c4++)
		{
			int nArc1 = Tracker.back().m_ActiveArcs[nPiece1][c4];

			int sz = (int)Tracker.back().m_ActiveArcs[nPiece2].size();
			for (int c5 = 0; c5 < sz; c5++)
			{
				int nArc2 = Tracker.back().m_ActiveArcs[nPiece2][c5];
				//DebugOutput("[%d %d] [%d %d]\n", nPiece1, nPiece2, nArc1, nArc2);
				PScores(nPiece1, nPiece2)(nArc1, nArc2) =
					SignatureSimilarity(Pieces[nPiece1].m_Arcs(nArc1).m_Signature,
						Orient_Reverse(Pieces[nPiece2].m_Arcs(nArc2).m_Signature),
						Dkappa);
			}
		}
		PScores(nPiece2, nPiece1) = PScores(nPiece1, nPiece2);

		//PScores.Display(nPiece1, nPiece2, pParams->m_P0[j]);
		if (pParams->m_bShowPScores)
			PScores.Display(pParams->m_P0[j]);
	}

}

void ComputeMuScore(CFit& fitc2, TwoPieces& piecesc2)
{
	if (fitc2.m_Score < 0)						// No score, yet
	{
		// ***BUGBUG***  or maybe not.  Transforms are always zero'ed when created

		//fitc2.m_ArcTrans = MatrixX3d::Zero(fitc2.m_Arcs.rows(), 3);

		Vector2d Theta(0.0, 0.0);

		fitc2.m_gFit.dx = 0; fitc2.m_gFit.dy = 0; fitc2.m_gFit.theta = 0;
		fitc2.m_ArcTrans.resize(fitc2.m_Arcs.rows());

		// Compute the average angle of rotation.  To compute the average, we use the Mean of Angles formula:
		//
		// ThetaBar = arg( sum(e^(i*Thetaj)) )
		//


		for (int c3 = 0; c3 < fitc2.m_Arcs.rows(); c3++)
		{
			const Curve& curve1 = Pieces[piecesc2[0]].m_Arcs(fitc2.m_Arcs(c3, 0)).m_Contour;

			Curve curve2 = TransformCurve(
				Pieces[piecesc2[1]].m_Arcs(fitc2.m_Arcs(c3, 1)).m_Contour,
				Placements[Tracker.back().m_Pc2Place[piecesc2[1]]].m_gLock);
			curve2.colwise().reverseInPlace();

			const Curve& sig1 = Pieces[piecesc2[0]].m_Arcs(fitc2.m_Arcs(c3, 0)).m_Signature;
			Curve sig2 = Orient_Reverse(Pieces[piecesc2[1]].m_Arcs(fitc2.m_Arcs(c3, 1)).m_Signature);

			fitc2.m_ArcTrans[c3].theta = Rigid_Motion_Angle(curve1, curve2, sig1, sig2, pParams->m_nBeta);

		}

		fitc2.MeanOfAngles();
		CalcTranslation(fitc2, Placements, Tracker);
	}
}

void TryShortening(CFit& fitc2, int j, int& c2)
{
	bool bWeird = false;

	//% In this case, it may be that a subset of this collection
	//% of arc pairings will have a high enough mu - score.
	//% We need only check this possibility if trimming an arc
	//% will not drop the number of arc pairs below m_0

	if (fitc2.m_Size > pParams->m_M0[j])
	{
		//% Create a temporary structure

		vector<CFit> tFits;

		//% We generate smaller sequences of matched pairings by
		//% deleting the first and last arcs of Fits(c2).  Note,
		//% however, that we only consider the result of deleting
		//% the last arc if Fits(c2) did not arise from deleting
		//% the first arc of a larger fit(i.e., if Fits(c2).Slot
		//% == 1).This avoids generating duplicate fits.

		//% Delete last arc

		if (fitc2.m_Slot)
		{
			tFits.resize(1);
			CFit& fit = tFits[0];

			fit.m_Pieces = fitc2.m_Pieces;

			fit.m_Size = fitc2.m_Size - 1;
			fit.m_Arcs = fitc2.m_Arcs.topRows(fit.m_Size);
			fit.m_ArcTrans = fitc2.m_ArcTrans;
			fit.m_ArcTrans.pop_back();

			CalcAverageTheta(fit);
			//if (bWeird)
			//	__debugbreak();
			CalcTranslation(fit, Placements, Tracker);
			if (bWeird && fit.m_Score == 0)
				__debugbreak();

			fit.m_Slot = 1;
		}

		//% Delete first arc

		{
			tFits.resize(tFits.size() + 1);
			CFit& fit = tFits.back();

			fit.m_Pieces = fitc2.m_Pieces;
			fit.m_Size = fitc2.m_Size - 1;
			fit.m_Arcs = fitc2.m_Arcs.bottomRows(fit.m_Size);
			fit.m_ArcTrans = fitc2.m_ArcTrans;
			fit.m_ArcTrans.erase(fit.m_ArcTrans.begin());

			CalcAverageTheta(fit);
			//if (bWeird)
			//	__debugbreak();
			CalcTranslation(fit, Placements, Tracker);
			if (bWeird && fit.m_Score == 0)
				__debugbreak();

			fit.m_Slot = 0;
		}

		//% The new fits in tFits are shorter, so we must figure
		//% out where to place them within Fits in order to
		//% preserve the sorting on that structure

		int c3 = c2 + 1;
		while (c3 < Fits.size() && Fits[c3].m_Size == Fits[c2].m_Size)
			c3++;

		// % Insert tFits into Fits.

		if (Fits[1].m_Pieces[0] < 0)
			__debugbreak();

		if (c3 < Fits.size())
			Fits.insert(Fits.begin() + c3, tFits.begin(), tFits.end());
	}

	if (Fits[1].m_Pieces[0] < 0)
		__debugbreak();

	//% Delete the fit that has now been subdivided.

	Fits.erase(Fits.begin() + c2);

	//% Update counter variable

	if (Fits[1].m_Pieces[0] < 0)
		__debugbreak();

	c2--;
}

void FindConsecutive(int& nPiece1, int& nPiece2, int j)
{
	MatrixXi included = MatrixXi::Zero(PScores(nPiece1, nPiece2).rows(), PScores(nPiece1, nPiece2).cols());
	vector<Vector2i> tArcs;

	vector<int>& aarcs1 = Tracker.back().m_ActiveArcs[nPiece1];
	vector<int>& aarcs2 = Tracker.back().m_ActiveArcs[nPiece2];
	size_t sz1 = aarcs1.size();
	size_t sz2 = aarcs2.size();

	for (int c4 = 0; c4 < sz1; c4++)
	{
		int nArc1 = aarcs1[c4];


		for (int c5 = 0; c5 < sz2; c5++)
		{
			int nArc2 = aarcs2[c5];

			if (!included(nArc1, nArc2)
				&& PScores(nPiece1, nPiece2)(nArc1, nArc2) >= pParams->m_P0[j])
			{
				included(nArc1, nArc2) = 1;

				tArcs.resize(0);
				tArcs.push_back(Vector2i(nArc1, nArc2));

				// Look c6 elements ahead in aarcs1 and c6 elements behind in aarcs2.

				int c6 = 1;													// Offset, not an index

				while (PScores(nPiece1, nPiece2)(aarcs1[(c4 + c6) % sz1], aarcs2[(c5 - c6 + sz2) % sz2]) >= pParams->m_P0[j])
				{
					// If there are non-consecutive arcs, we're done

					if (abs(aarcs1[(c4 + c6) % sz1] - aarcs1[(c4 + c6 - 1) % sz1]) > 1
						|| abs(aarcs2[(c5 - c6 + sz2) % sz2] - aarcs2[(c5 - c6 + 1 + sz2) % sz2]) > 1)
						break;
					else
					{
						int a1 = (aarcs1[c4] + c6) % included.rows();
						int a2 = (int)((aarcs2[c5] - c6 + 2 * included.cols()) % included.cols());

						tArcs.push_back(Vector2i(a1, a2));
						included(a1, a2) = 1;

						c6++;
					}
				}

				// Look c7 elements behind in aarcs1 and c6 elements ahead in aarcs2.

				int c7 = 1;

				while (PScores(nPiece1, nPiece2)(aarcs1[(c4 - c7 + sz1) % sz1], aarcs2[(c5 + c7) % sz2]) >= pParams->m_P0[j])
				{
					int atest1 = aarcs1[(c4 - c7 + sz1) % sz1];
					int atest2 = aarcs1[(c4 - c7 + 1 + sz1) % sz1];
					if (abs(atest1 - atest2) > 1)
						break;

					atest1 = aarcs2[(c5 + c7) % sz2];
					atest2 = aarcs2[(c5 + c7 - 1) % sz2];
					if (abs(atest1 - atest2) > 1)
						break;

					{
						int a1 = (int)((aarcs1[c4] - c7 + included.rows()) % included.rows());
						int a2 = (int)((aarcs2[c5] + c7) % included.cols());

						tArcs.insert(tArcs.begin(), Vector2i(a1, a2));
						included(a1, a2) = 1;

						c7++;
					}
				}
				int minIncluded = (int)min(included.rows(), included.cols());
				if ((c6 + c7 - 1 - minIncluded) > 0)
				{
					for (int c8 = 0; c8 < c6 + c7 - 1 - minIncluded; c8++)
					{
						MatrixXi x;
						CFit fit;
						fit.m_Size = minIncluded;
						fit.m_Pieces = TwoPieces(nPiece1, nPiece2);
						Map<Matrix<int, -1, 2, RowMajor> > t((int*)tArcs.data(), tArcs.size(), 2);
						fit.m_Arcs = t.block(c8, 0, minIncluded, 2);
						fit.m_Slot = 1;
						fit.m_Score = -1.0;		// Not yet computed
						Fits.push_back(fit);
					}
				}
				else
				{
					CFit fit;
					fit.m_Size = c6 + c7 - 1;
					fit.m_Pieces = TwoPieces(nPiece1, nPiece2);
					Map<Matrix<int, -1, 2, RowMajor> > t((int*)tArcs.data(), tArcs.size(), 2);
					fit.m_Arcs = t;
					fit.m_Slot = 1;
					fit.m_Score = -1.0;		// Not yet computed
					Fits.push_back(fit);
				}
				if (Fits.back().m_Size < pParams->m_M0[j])
					Fits.pop_back();
			}
		}
	}
}

bool TryLock(int c2, CFit& fitc2, GTransform& gLock, int j, Indices& tPiecePtIcs, LRESULT& plotHandle, TwoPieces& piecesc2, Vector5d& score)
{
	Indices tPiecePtIcs_3;
	double q1, q2, q3, q4;

	DebugOutput("Lock c2=%d: Piece %d, %d points.  SPB has %d points  theta=%.4f\n", c2, fitc2.m_Pieces[0], Pieces[fitc2.m_Pieces[0]].m_Contour.rows(), Tracker.back().m_SolvedPuzzleBoundary.rows(), fitc2.m_gFit.theta);

	Lock(fitc2, Pieces[fitc2.m_Pieces[0]].m_Contour, Tracker.back().m_SolvedPuzzleBoundary,
		gLock, pParams->m_K3[j], tPiecePtIcs_3, Tracker.back().m_SP_Bdry_PtIcs_3, tPiecePtIcs, Tracker.back().m_SP_Bdry_PtIcs, pParams->m_szPlotLevel[0] > '2', plotHandle);

	// Compute Scores

	q1 = 1.0 * Tracker.back().m_SP_Bdry_PtIcs_3.size() / Tracker.back().m_SP_Bdry_PtIcs.size();
	DebugOutput("m_SP_Bdry_PtIcs_3 has %d points,  m_SP_Bdry_PtIcs has %d points.  q1 = %.3f\n", Tracker.back().m_SP_Bdry_PtIcs_3.size(), Tracker.back().m_SP_Bdry_PtIcs.size(), q1);

	q2 = 0;
	int tn = (int)tPiecePtIcs_3.size();
	for (int c3 = 0; c3 < tn; c3++)
	{
		int c3p1 = (c3 + 1) % tn;
		int diff = abs(tPiecePtIcs_3[c3p1] - tPiecePtIcs_3[c3]);
		if (diff == 1 || diff == (Pieces[piecesc2[0]].m_Contour).rows() - 1)
		{
			q2 += (Pieces[piecesc2[0]].m_Contour.row(tPiecePtIcs_3[c3p1]) - Pieces[piecesc2[0]].m_Contour.row(tPiecePtIcs_3[c3])).norm();
		}
	}
	q2 /= AverageLength;
	q3 = Gather(Pieces[piecesc2[0]].m_Signature, tPiecePtIcs_3).col(0).array().abs().sum() / (Dkappa * AverageSize);

	q4 = 0.0;

	if (!((pParams->m_dEta1 * q1 + pParams->m_dEta2 * q3 > pParams->m_dQ1 || q2 > pParams->m_dQ2Star) && q2 > pParams->m_dQ2 && q3 > pParams->m_dQ3))
	{
		score << q1, q2, q3, q4, pParams->m_dEta1* q1 + pParams->m_dEta2 * q3;
		return false;
	}

	score << q1, q2, q3, q4, pParams->m_dEta1* q1 + pParams->m_dEta2 * q3;
	return (pParams->m_dEta1 * q1 + pParams->m_dEta2 * q3 > pParams->m_dQ1 || q2 > pParams->m_dQ2Star) && q2 > pParams->m_dQ2 && q3 > pParams->m_dQ3;
}

void PlotAllPlacements(const LRESULT& fragh, int c1, LRESULT& farch1, LRESULT& farch2)
{
	Progress.Delete(fragh);
	/******/Progress.Erase(true);
	Progress.Plot(Tracker.back().m_SolvedPuzzleBoundary, RGB(0, 0, 255), 3);

	for (int i = 0; i <= c1; i++)
	{
		PlotPlacement(Placements[i]);

		//Progress.Plot(TransformCurve(Pieces[Placements[c1].m_nPiece].m_Contour, gLock));

#ifdef _DEBUG
		//Sleep(500);
#endif
		// Update fit arc points

		CFit& fit = Placements[i].m_Fit;
		Curve farcpts1, farcpts2;

		for (int c4 = 0; c4 < fit.m_Arcs.rows(); c4++)
		{
			Curve x1 = TransformCurve(Pieces[fit.m_Pieces[0]].m_Arcs(fit.m_Arcs(c4, 0)).m_Contour,
				Placements[Tracker.back().m_Pc2Place[fit.m_Pieces[0]]].m_gLock);
			Curve x2 = TransformCurve(Pieces[fit.m_Pieces[1]].m_Arcs(fit.m_Arcs(c4, 1)).m_Contour,
				Placements[Tracker.back().m_Pc2Place[fit.m_Pieces[1]]].m_gLock);

			Append(farcpts1, x1);
			Append(farcpts2, x2);
		}

		//if (farch1)
		//	Progress.Delete(farch1);
		//if (farch2)
		//	Progress.Delete(farch2);

		if (farcpts1.rows() != 0)
			farch1 = Progress.Plot(farcpts1, RGB(255, 0, 255), -3);
		if (farcpts2.rows() != 0)
			farch2 = Progress.Plot(farcpts2, RGB(255, 0, 0), -3);
	}

	Progress.Unhold();
}

void PlacePieces()
{
	size_t nPieces = Pieces.size();
	PScores.SetSize(nPieces);
	int MaxFits = 0;

	if (Placements.size() == 0)
	{
		Eigen::VectorXd weights(nPieces);

		Tracker.resize(1);

		for (int i = 0; i < nPieces; i++)
		{
			Pieces[i].m_nActive = 0;
			weights[i] = Pieces[i].m_Weight;
		}

		int piece1;							// Start with the "heaviest" piece.

		double dummy = weights.maxCoeff(&piece1);
		Pieces[piece1].m_nActive = 1;

		Tracker.back().SetSize(nPieces);

		Tracker.back().m_PlacedPieces.resize(1);
		Tracker.back().m_PlacedPieces[0] = piece1;

		Tracker.back().m_SolvedPuzzleBoundary = Pieces[piece1].m_Contour;	// Initially, the solved puzzle is just piece1's contour

		Tracker.back().m_SPB_Pt2PcPt.resize(Pieces[piece1].m_Contour.rows(), 2);
		Tracker.back().m_SPB_Pt2PcPt.col(0).setConstant(piece1);
		size_t sz = Pieces[piece1].m_Contour.rows();
		for (int i = 0; i < sz; i++)
			Tracker.back().m_SPB_Pt2PcPt(i, 1) = i;


		for (int i = 0; i < nPieces; i++)
		{
			Tracker.back().m_RemainingPieces[i] = i;

			Tracker.back().m_ActiveArcs[i].resize(Pieces[i].m_Arcs.size());
			size_t sz = Pieces[i].m_Arcs.size();
			for (int j = 0; j < sz; j++)
				Tracker.back().m_ActiveArcs[i][j] = j;

			Tracker.back().m_ActivePoints[i].resize(Pieces[i].m_Contour.rows());
			sz = Pieces[i].m_Contour.rows();
			for (int j = 0; j < sz; j++)
				Tracker.back().m_ActivePoints[i][j] = j;

			Tracker.back().m_Pc2Place[i] = (i == piece1) - 1;
		}

		Tracker.back().m_RemainingPieces.erase(Tracker.back().m_RemainingPieces.begin() + piece1);

		Placements.emplace_back(piece1, Vector5d(0.0, 0.0, 0.0, 0.0, 0.0), GTransform(), CFit());
	}

	Progress.NewScores();

	int c1 = (int)Placements.size();
	int nRPc = (int) nPieces - c1 + 1;

	int j = 0;																// Parameter sequence index

// c1	counts # of placed pieces
// c2	counts unplaced pieces.  Index into tracker.m_RemainingPieces
// c3	counts placed pieces.  Index into tracker.m_PlacedPieces

	Progress.RestartReport(PROGRESS_PLACING, true);

	LRESULT plotHandle = 0;
	LRESULT fragh = 0;
	LRESULT farch1 = 0;
	LRESULT farch2 = 0;

	if (pParams->m_szPlotLevel[0] > '1')
	{
		Progress.Erase(true);
		fragh = Progress.Plot(Tracker.back().m_SolvedPuzzleBoundary, RGB(0, 0, 255), 3);

		for (int i = 0; i < Placements.size(); i++)
			PlotPlacement(Placements[i]);

		Progress.Unhold();
	}

	// ***NOTE*** H&O plots all placements here.  I assume this is used when resuming a saved puzzle.

	while (c1 < nPieces)
	{
		Fits.resize(0);

		Progress.RestartReport(PROGRESS_COMPARING, true);

		for (int c2 = 0; c2 < (nPieces - c1); c2++)
		{
			int nPiece1 = Tracker.back().m_RemainingPieces[c2];
			for (int c3 = 0; c3 < c1; c3++)
			{
				//% --------------------------------------------------------------------------
				//% Find P-Scores between arcs of remaining pieces and arcs of placed pieces
				//%--------------------------------------------------------------------------

				int nPiece2 = Tracker.back().m_PlacedPieces[c3];

				ComputePScores(nPiece1, nPiece2, j);

				//%--------------------------------------------------------------------------
				//% Find sequences of consecutive high P-Scores

				//%--------------------------------------------------------------------------

				FindConsecutive(nPiece1, nPiece2, j);

				Progress(PROGRESS_COMPARING).m_Percent = 1.0*(c1*c2 + c3) / (c1*(nPieces - c1));
				Progress.UpdateReport();
				
			}
		}

		if (pParams->m_bDumpFitBeforeQSort)
			CFit::Dump("Before QSort");

		qsort(Fits.data(), Fits.size(), sizeof(CFit), [](const void* p1, const void* p2)->int
		{
			CFit* pFit1 = (CFit*)p1;
			CFit* pFit2 = (CFit*)p2;
			int nRes = (pFit2->m_Size - pFit1->m_Size);				// Descending sort on size
			if (nRes) return nRes;
			return pFit1->m_Pieces[0] - pFit2->m_Pieces[0];			// For same size, ascending by Pieces[0]
		});

		if (pParams->m_bDumpFitBeforeQSort)
			CFit::Dump("After QSort");

		int c2 = 0;
		bool bProgress = false;

		Progress.RestartReport(PROGRESS_CHECKING, true);

		// Check fits

		nFitSteps.resize(Fits.size());
		for (int& i : nFitSteps) i = 0;
		for (int& i : nFitSteps) cout << i;

		while (c2 < Fits.size())
		{
			CFit& fitc2 = Fits[c2];
			TwoPieces& piecesc2 = fitc2.m_Pieces;

			if (Pieces[piecesc2[1]].m_nActive &&
				IsMember(piecesc2[0], Tracker.back().m_RemainingPieces) &&
				!AnyMatch(fitc2.m_Arcs.col(1), Tracker.back().m_InactiveArcs[piecesc2[1]]))
			{
			//% --------------------------------------------------------------------------
			//% Calculate transformation and mu - score
			//% --------------------------------------------------------------------------

				ComputeMuScore(fitc2, piecesc2);

				if (fitc2.m_Score >= 0 && fitc2.m_Score < pParams->m_MU0[j])
				{
					int temp = c2;
					TryShortening(fitc2, j, c2);

					if (pParams->m_bDumpFitBeforeQSort)
					{
						char szBuff[MAX_PATH];
						nFitSteps[temp]++;
						sprintf_s(szBuff, sizeof(szBuff), "%s-%03d-", "After Shortening", temp);
						CFit::Dump(szBuff);
					}
				}
				else
				{
					GTransform gLock;
					Indices tPiecePtIcs;
					Vector5d score;

#ifdef _DEBUG
					int p0 = fitc2.m_Pieces[0];
					if (p0 == 55)
					{
						fitc2.m_gFit.theta = 2.205695344892561;
						fitc2.m_gFit.dx = 3.425508042156975e+02;
						fitc2.m_gFit.dy = -8.708948917850823e+02;
						pParams->m_szPlotLevel[0] = '3';
					}
#endif

					bool bLockSucceeded = TryLock(c2, fitc2, gLock, j, tPiecePtIcs, plotHandle, piecesc2, score);

					if (bLockSucceeded && pParams->m_bConfirmBeforePlacing)
					{
						auto h = Progress.Plot(TransformCurve(Pieces[fitc2.m_Pieces[0]].m_Contour, gLock));

						char buff[1024];
						sprintf_s(buff, sizeof(buff), "%d: %.3f  %.3f  %.3f  %.3f  %3f", fitc2.m_Pieces[0], score(0), score(1), score(2), score(3),score(4));

						auto res = Progress.ConfirmPlacement(buff);
						if (res == IDNO)
						{
							AlwaysOutput("%s\n", buff);
							bLockSucceeded = false;
						}
						Progress.Delete(h);
					}

					// Update progress report

					//pParams->m_szPlotLevel[0] = '2';

					Progress(PROGRESS_CHECKING).m_Percent = 1.0*c2 / Fits.size();
					Progress.UpdateReport();

					// If scores are high enough, place the piece

					if (bLockSucceeded)
					{
						// Add new placement to placements variable
						DebugOutput("Lock success\n");

						assert(c1 == Placements.size());

						Placements.emplace_back(fitc2.m_Pieces[0], score, gLock, fitc2);

						// Update tracker variable

						Tracker.push_back(Tracker.back());
						Tracker.back().m_Pc2Place[Placements[c1].m_nPiece] = c1;

						// Mark arcs / points as used or remaining and introduce neighbors / activate

						vector<int>& inactive = Tracker.back().m_InactiveArcs[piecesc2[0]];
						int prev = -1;

						for (int c3 = 0; c3 < tPiecePtIcs.size(); c3++)
						{
							auto arcno = Pieces[piecesc2[0]].m_Pt2Arc(tPiecePtIcs[c3]);
							if (arcno != -1 && arcno != prev)
							{
								inactive.push_back(arcno);
								prev = arcno;
							}
						}

						Placements[c1].m_Neighbors.resize(0);
						Pieces[fitc2.m_Pieces[0]].m_nActive = 2;

						vector<int>& neighbors = Placements[c1].m_Neighbors;

						for (int c3 = 0; c3 < Tracker.back().m_SP_Bdry_PtIcs.size(); c3++)
						{
							int SPIc3 = Tracker.back().m_SP_Bdry_PtIcs[c3];

							// Introduce new neighbors

							int newNeighbor = Tracker.back().m_SPB_Pt2PcPt(SPIc3, 0);

							if (!IsMember(newNeighbor, neighbors))
							{
								neighbors.push_back(newNeighbor);
								Pieces[newNeighbor].m_nActive = 2;

								vector<int>& toSetActive = Placements[Tracker[c1].m_Pc2Place[newNeighbor]].m_Neighbors;
								size_t sz = toSetActive.size();
								for (int i = 0; i < sz; i++)
									Pieces[toSetActive[i]].m_nActive = 2;

								toSetActive.push_back(fitc2.m_Pieces[0]);

							}

							// Mark arcs as inactive
							
							auto xxx = Tracker.back().m_SPB_Pt2PcPt(SPIc3, 1);
							auto arc = Pieces[newNeighbor].m_Pt2Arc(xxx);

							if (arc)
							{
								Tracker.back().m_InactiveArcs[newNeighbor].push_back(arc);
							}

							// Mark points as inactive

							Tracker.back().m_InactivePoints[newNeighbor].push_back(Tracker.back().m_SPB_Pt2PcPt(SPIc3, 1));

						}

						auto& fitc2piece = fitc2.m_Pieces[0];

						Tracker.back().m_InactivePoints[fitc2piece] = tPiecePtIcs;

						// Note: Lock returns tPiecePtIcs in sorted order.  Convenient!
						//
						// ActivePoints is the complement of the InactivePoints

						vector<int>& activePoints = Tracker.back().m_ActivePoints[fitc2piece];

						size_t sz = Pieces[fitc2piece].m_Contour.rows();
						for (int i = 0; i < sz; i++)
							activePoints[i] = i;

						for (int i = (int) tPiecePtIcs.size() - 1; i >= 0; i--)
							activePoints.erase(activePoints.begin() + tPiecePtIcs[i]);

						// Same with arcs, except might not be sorted and will have duplicates!

						vector<int>& activeArcs = Tracker.back().m_ActiveArcs[fitc2piece];

						AllExcept(activeArcs, Pieces[fitc2piece].m_Arcs.rows(), Tracker.back().m_InactiveArcs[fitc2piece]);

						for (int c3 = 0; c3 < c1 - 1; c3++)
						{
							int placedPiece = Tracker.back().m_PlacedPieces[c3];
							vector<int>& activeArcs = Tracker.back().m_ActiveArcs[placedPiece];
							vector<int>& activePoints = Tracker.back().m_ActivePoints[placedPiece];

							// Calculate AArcs

							AllExcept(activeArcs, Pieces[placedPiece].m_Arcs.size(), Tracker.back().m_InactiveArcs[placedPiece]);

							// Calculate APts

							AllExcept(activePoints, Pieces[placedPiece].m_Contour.rows(), Tracker.back().m_InactivePoints[placedPiece]);
						}

						// Calculate new Fragment

						// Remove elements from the solved puzzle boundary (H&O line 634)

						Tracker.back().m_SolvedPuzzleBoundary = RemoveElements(Tracker.back().m_SolvedPuzzleBoundary, Tracker.back().m_SP_Bdry_PtIcs);

						Curve tPiece = Pieces[fitc2.m_Pieces[0]].m_Contour;
						vector<int> tTrack;
						AllExcept(tTrack, tPiece.rows(), tPiecePtIcs);
						tPiece = RemoveElements(tPiece, tPiecePtIcs);

						Append(Tracker.back().m_SolvedPuzzleBoundary, TransformCurve(tPiece, gLock));

						Tracker.back().m_SPB_Pt2PcPt = RemoveElements(Tracker.back().m_SPB_Pt2PcPt, Tracker.back().m_SP_Bdry_PtIcs);

						MatrixX2i tTrackPlus(tTrack.size(), 2);
						tTrackPlus.col(0).setConstant(Placements[c1].m_nPiece);

						int szx = (int) tTrack.size();
						for (int i = 0; i < szx; i++)
						{
							tTrackPlus(i, 1) = tTrack[i];
						}

						Append(Tracker.back().m_SPB_Pt2PcPt, tTrackPlus);

						// Mark pieces as used or remaining

						Tracker.back().m_PlacedPieces.push_back(Placements[c1].m_nPiece);
						AllExcept(Tracker.back().m_RemainingPieces, nPieces, Tracker.back().m_PlacedPieces);

						Progress(PROGRESS_PLACING).m_Percent= 1.0 * (c1 - nPieces + nRPc) / nRPc;
						Progress.UpdateReport();

						PlotAllPlacements(fragh, c1, farch1, farch2);
						//Sleep(500);

						bProgress = true;

						if (pParams->m_bDumpFitBeforeQSort)
							CFit::Dump("After Placement");

						if (pParams->m_bSave)
							Progress.SavePuzzle();

						c1++;
					}

					Progress.NewScores();

					if (plotHandle)
						Progress.Delete(plotHandle);
					plotHandle = 0;
				}
			}
			c2++;
		}
		
		// End of Check Fits loop

		Progress.RestartReport(PROGRESS_CHECKING, false);
		Progress(PROGRESS_COMPARING).m_Percent = 0;
		Progress.UpdateReport();

		// If the algorithm has dead - ended, increase depth in parameter sequence
		// or terminate algorithm if this is not possible

		if (!bProgress)
		{
			if (j < pParams->m_nJStar)
			{
				j++;

				// Activate pieces

				size_t sz = Tracker.back().m_PlacedPieces.size();
				for (int c4 = 0; c4 < sz; c4++)
				{
					Pieces[Tracker.back().m_PlacedPieces[c4]].m_nActive = 1;
				}
			}
			else
			{
				// ***BUGBUG*** Dead End!  Also, plot!
				__debugbreak();
				return;
			}
		}
		else
		{
			j = 1;
			// Deactivate pieces
			size_t sz = Tracker.back().m_PlacedPieces.size();
			for (int c4 = 0; c4 < sz; c4++)
			{
				Pieces[Tracker.back().m_PlacedPieces[c4]].m_nActive = 
					max(0, Pieces[Tracker.back().m_PlacedPieces[c4]].m_nActive - 1);
			}
		}

		/*
		*/
	}
}


// Find and eliminate outliers at either end of arc

void CFit::MeanOfAngles()
{
	CalcAverageTheta(*this);

	//double thetaAvg = m_gFit.theta;
	//if (thetaAvg < 0)
	//	thetaAvg += 2 * PI;

	//// Now look at the 1st and last arcs and check for outliers

	//int nStart = 0;
	//int nEnd = (int)m_ArcTrans.size() - 1;

	//for (int i = 0; i < m_ArcTrans.size(); i++)
	//{
	//	double dErr = fabs(m_ArcTrans[i].theta - thetaAvg) / thetaAvg;
	//	if (dErr > 0.15)
	//	{
	//		nStart = i + 1;
	//	}
	//	else
	//		break;
	//}

	//for (int i = (int)m_ArcTrans.size() - 1; i >= 0; i--)
	//{
	//	double dErr = fabs(m_ArcTrans[i].theta - thetaAvg) / thetaAvg;
	//	if (dErr > 0.15)
	//	{
	//		nEnd = i - 1;
	//	}
	//	else
	//		break;
	//}

	//if (nStart < m_Size && nEnd >= 0)
	//{
	//	// Now, delete all the arcs we don't want

	//	m_Arcs = m_Arcs.block(nStart, 0, (nEnd - nStart) + 1, 2).eval();

	//	if (nStart > 0)
	//	{
	//		memcpy_s(&m_ArcTrans[0], sizeof(m_ArcTrans[0])*m_ArcTrans.size(), &m_ArcTrans[nStart], sizeof(m_ArcTrans[0])*(m_ArcTrans.size() - nStart));
	//	}

	//	m_Size = (nEnd - nStart) + 1;
	//	m_ArcTrans.resize(m_Size);
	//}

	//CalcAverageTheta(*this);
}

