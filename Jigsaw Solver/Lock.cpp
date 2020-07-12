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

using VectorXb = Matrix<bool, -1, 1>;

/*
function new_points = transf(points, trans, cm)

% This function applies a rigid motion to inputed points
%{
%--------------------------------------------------------------------
INPUTS
%--------------------------------------------------------------------

'points':   This should be n points represented as an n-by-2 matrix,
			each row of which specifies a point in R^2.

'trans':    This input should be a matrix of the form [theta a b]
			where these parameters specify a rigid motion consisting
			of a translation by [a b] and a rotation of theta radians
			around the point cm.

'cm':       The point in R^2, represented by a 1-by-2 matrix, about
			which the rotation will take place.

%--------------------------------------------------------------------


%--------------------------------------------------------------------
OUTPUTS
%--------------------------------------------------------------------

'new_points':   This output gives the transformed points as an n-by-2
			matrix, each row of which specifies a point in R^2.

%--------------------------------------------------------------------
%}
*/

inline static Matrix<double, 1, 2>
 TransformPointAboutCM(Vector2d point, const GTransform& trans, Vector2d cm)
{
	Vector2d result;
	Vector2d xx = point.array() - cm.array();

	double c = cos(trans.theta);
	double s = sin(trans.theta);

	result[0] = (xx(0) * c - xx(1) * s) + trans.dx + cm[0];
	result[1] = (xx(0) * s + xx(1) * c) + trans.dy + cm[1];

	return result;
}

inline static MatrixX2d
TransformCurveAboutCM(MatrixX2d curve, const GTransform& trans, Vector2d cm)
{
	MatrixX2d result;
	result.resizeLike(curve);

	MatrixX2d xx = curve.rowwise() - cm.transpose();

	double c = cos(trans.theta);
	double s = sin(trans.theta);

	result.col(0) = (xx.col(0) * c - xx.col(1) * s).array() + trans.dx + cm[0];
	result.col(1) = (xx.col(0) * s + xx.col(1) * c).array() + trans.dy + cm[1];

	//result[0] = (xx(0) * c - xx(1) * s) + trans.dx + cm[0];
	//result[1] = (xx(0) * s + xx(1) * c) + trans.dy + cm[1];

	return result;
}

/*
% This function performs the locking algorithm described in [1]. It
% is intended for use by the Assemble() function.
%{
%--------------------------------------------------------------------
INPUTS
%--------------------------------------------------------------------

'g_0':      This input should be a matrix of the form [theta a b]
			where these parameters specify a rigid motion as
			described in [2] (a translation by [a b] and a rotation
			by theta radians around the origin).

'C_Delta':  This should be a discretized planar curve of n points
			represented as an n-by-2 matrix, each row of which
			specifies a point in R^2. The curve is assumed to be
			closed; no repetition of points is necessary.

'Ctilde_Delta': This should be a discretized planar curve of n points
			represented as an n-by-2 matrix, each row of which
			specifies a point in R^2. The curve is assumed to be
			closed; no repetition of points is necessary.

'K_1', 'K_2', 'K_3', 'K_4', 'epsilon', 'nu', 'rho', 'j_max':   These inputs
			should be doubles that give the parameters as described
			in [1].

'plotter':  In this bool, value of 1 results in the plotting of a
			visualization of the piece locking whereas a value of 0
			does not.

'fh':       If this input is supplied and plotter = 1, the
			visualization is plotted on the figure with handle 'fh'.

%--------------------------------------------------------------------


%--------------------------------------------------------------------
OUTPUTS
%--------------------------------------------------------------------

'g_lock'	: This is the resulting transformation as described in
			[1].

'D_Delta_3_Ics', 'Dtilde_Delta_3_Ics', 'D_Delta_2_Ics',
'Dtilde_Delta_2_Ics': These columns contain the
			indices (within C_Delta and Ctilde_Delta) of
			the points in the sets as defined in [1].

'xph':      This output is assigned only if 'fh' is inputted and
			plotter = 1, in which case it contains the handle of an
			object to be later deleted by the Assemble() function.

%--------------------------------------------------------------------
%}
*/

void
Lock(CFit& fit, const Curve& CDelta, const Curve& CtildeDelta, GTransform& gLock, double K3, Indices& D_Delta_3_Ics, Indices& Dtilde_Delta_3_Ics, Indices& D_Delta_2_Ics, Indices& Dtilde_Delta_2_Ics, bool bPlot, LRESULT& plotHandle)
{
	GTransform& g0 = fit.m_gFit;

	Pauser.Lock();
	Pauser.Unlock();

	int nC = (int)CDelta.rows();
	int nCtilde = (int)CtildeDelta.rows();
	Curve CDeltam1;
	vector<Curve> EtildeDeltaK1;
	Curve EDeltaK1;
	vector<int> EDeltaK1Ics;
	double thetaj = 0.0;
	Vector2d cj(0.0, 0.0);

	LRESULT inith;
	LRESULT debugh = 0;
	LRESULT debughtilde = 0;

	if (bPlot)
		inith = Progress.Plot(TransformCurve(CDelta, g0));

	circShift(CDelta, CDeltam1, -1);

	// Equation 5.3

	double dStar = ((CDelta.col(0) - CDeltam1.col(0)).array().square() + (CDelta.col(1) - CDeltam1.col(1)).array().square()).sqrt().sum() / nC;
	double dStarK1 = pParams->m_dK1*dStar;

	// Equations 5.4
	//
	// bNearby is a boolean matrix.  It has CtildeDelta.rows() rows by CDelta.rows() columns.  Thus, each row corresponds
	// to an element (ztilde) in CtildeDelta) and each column corresponds to an element in CDelta.
	//
	// An entry is true if || ztilde - g · z || < dStar * K1.  That is if the points are "close enough" to interact in the
	// locking alogorithm
	// 

	Matrix<bool, -1, -1> bNearby;
	bNearby.resize(CtildeDelta.rows(), CDelta.rows());

	FOR_START(c1, 0, nC)
		bNearby.col(c1) = (CtildeDelta.rowwise() - TransformPointAboutCM(CDelta.row(c1), g0, Vector2d(0, 0))).rowwise().norm().array() < dStarK1;
	FOR_END

		// The goal is now to create 2 sets: EDeltaK1 and EtildeDeltaK1.
		//
		// EDeltaK1			is the set of points from CDelta that interact with CtildeDelta.

		// EtildeDeltaK1	is a vector of sets of points.  The ith element of EtildeDeltaK1
		//					is the set of points that interact with the ith point of EDeltaK1.

		// bNearby(row,col)	Each column corresponds to a point in CDelta, each row to a point
		//					in CtildeDelta.

		// totsCol			is the number of trues in each column of bNearby.  It is the number
		//					of points in CtildeDelta that interact with the CDelta of that column.

		// totsRow			is the number of trues in each row of bNearby.  It is the number
		//					of points in CDelta that interact with the CtildeDelta of that row.

	Matrix<Index, 1, -1> totsCol = bNearby.colwise().count();
	int nEDeltas = (int)(totsCol.array() > 0).count();

	if (nEDeltas == 0)
	{
		gLock = GTransform();
		return;
	}

//#ifdef _DEBUG
	Matrix<Index, 1, -1> totsRow = bNearby.rowwise().count();
	int nEtildeDeltas = (int)(totsRow.array() > 0).count();

	DebugOutput("nEDeltas=%d  nEtildeDeltas=%d\n", nEDeltas, nEtildeDeltas);

	Curve debugEtildeDeltaK1;
	debugEtildeDeltaK1.resize(nEtildeDeltas, 2);

	int iDest = 0;
	for (int i = 0; i < CtildeDelta.rows(); i++)
	{
		if (totsRow[i] != 0)
		{
			debugEtildeDeltaK1.row(iDest++) = CtildeDelta.row(i);
		}
	}
//#endif

	EtildeDeltaK1.resize(nEDeltas);
	EDeltaK1.resize(nEDeltas, 2);
	EDeltaK1Ics.resize(0);
	int iEtildeDeltaK1 = 0;

	for (int c1 = 0; c1 < nC; c1++)
	{
		if (totsCol[c1] != 0)
		{
			EDeltaK1Ics.push_back(c1);

			EDeltaK1.row(iEtildeDeltaK1) = CDelta.row(c1);
			Curve& c = EtildeDeltaK1[iEtildeDeltaK1++];
			c.resize(totsCol[c1], 2);
			int idx = 0;
			for (int r1 = 0; r1 < nCtilde; r1++)
			{
				if (bNearby(r1, c1))
				{
					c.row(idx++) = CtildeDelta.row(r1);
				}
			}
		}
	}

	// My contribution: make sure all piece arc points under consideration lie within EdeltaK1


	// Calculate perturbation constants.  Equations 5.5 and 5.6

	RowVector2d zCM = EDeltaK1.colwise().sum() / nEDeltas;
	RowVectorXd normSquared = (EDeltaK1.rowwise() - zCM).rowwise().squaredNorm();
	double r2 = normSquared.sum();
	double rINF = normSquared.array().sqrt().maxCoeff();

	if (bPlot)
	{
		if (debugh)
			Progress.Delete(debugh);

		Curve debugEDK = TransformCurve(EDeltaK1, g0);
		debugh = Progress.Plot(debugEDK, RGB(255, 0, 0), -3);

//#ifdef _DEBUG
		if (debughtilde)
			Progress.Delete(debughtilde);

		debughtilde = Progress.Plot(debugEtildeDeltaK1, RGB(0, 255, 0), -3);
		Sleep(5000);
//#endif
	}

	// Convert g_0 from the form used in Assemble() (rotation around the
	// origin) to the form used in Lock() (rotation around the center of
	// mass

	double c = cos(g0.theta);
	double s = sin(g0.theta);
	Matrix2d rot;
	rot << c, -s, s, c;

	Vector2d zCMt = zCM.transpose();

	MatrixXd translation = rot * zCMt - zCMt + Vector2d(g0.dx, g0.dy);

	GTransform gj(g0.theta, translation(0), translation(1));
	RowVector2d wj = zCMt + translation;

	// Perform iterative pertubation

	double dj = dStarK1;
	double ThetaJm1 = 0.0;
	Vector2d cJm1(0.0, 0.0);
	double dStarK4 = pParams->m_dK4 * dStar;
	bool bSeenOneTerminationCondition = false;

	for (int j = 0; j < pParams->m_nJmax; j++)
	{
		double tauTotj = 0.0;
		RowVector2d fTotj(0.0, 0.0);
		VectorXd Aj(nEDeltas);

		for (int c2 = 0; c2 < nEDeltas; c2++)
		{
			RowVector2d EDeltaK1Transformed = TransformPointAboutCM(EDeltaK1.row(c2), gj, zCM);
			MatrixXd diff = EtildeDeltaK1[c2].rowwise() - EDeltaK1Transformed;
			VectorXd diffNorm = diff.rowwise().norm();
			Aj[c2] = diffNorm.minCoeff();

			Vector2d fj;

			if (Aj[c2] >= dStarK4)
			{
				//VectorXd denom = (diffNorm.array().pow(pParams->m_nNu) + pParams->m_dEpsilon).array() * diffNorm.array();
				//MatrixXd ffjj = diff.array().colwise() / denom.array();
				//Vector2d FJ = ffjj.colwise().sum();
				Vector2d fj = (diff.array().colwise() / (diffNorm.array().pow(pParams->m_nNu + 1) + pParams->m_dEpsilon * diffNorm.array())).colwise().sum();
				fTotj.array() += fj.array();
				RowVector2d t = EDeltaK1Transformed - wj;
				tauTotj += t[0] * fj[1] - t[1] * fj[0];		// 
			}
		}

		// Equation 5.10

		double dAvj = Aj.mean();
		qsort(Aj.data(), Aj.rows(), sizeof(double), [](const void* p1, const void* p2) -> int
		{
			return (int)(*(double*)p1 - *(double*)p2);
		});

		double dMedj = Aj.rows() & 1 ? Aj[Aj.rows() / 2] : (Aj[Aj.rows() / 2 - 1] + Aj[Aj.rows() / 2]) / 2;

		DebugOutput("dAvj=%f  dMedj=%f\n", dAvj, dMedj);

		// Terminate the algorithm if fit is poor and getting worse.

		if (dAvj > dj && dMedj >= K3 * dStar)
		{
			gj.theta -= thetaj;
			gj.dx -= cj[0];
			gj.dy -= cj[1];

			if (bPlot)
			{
				if (plotHandle)
					Progress.Delete(plotHandle);

				Curve c = TransformCurveAboutCM(CDelta, gj, zCM);

				plotHandle = Progress.Plot(c);
				Sleep(100);
			}

			break;
		}

		// Calculate new transformation.  Equation 5.12

		double deltaj = pParams->m_dRho * dMedj / max(fTotj.norm() / nEDeltas, rINF*fabs(tauTotj) / r2);

		// Equations 5.11

		thetaj = deltaj * tauTotj / r2;
		cj = deltaj * fTotj / nEDeltas;

		// Update key quantities - Step 7

		gj.theta += thetaj;
		gj.dx += cj[0];
		gj.dy += cj[1];
		wj += cj;
		dj = dAvj;

		if (bPlot)
		{
			if (plotHandle)
				Progress.Delete(plotHandle);

			Curve c = CDelta.rowwise() - zCM;
			c = TransformCurve(c, gj);
			c.rowwise() += zCM;

			plotHandle = Progress.Plot(c);
			Sleep(100);
		}

		// Step 7, termination condition

		if ((thetaj * ThetaJm1 < 0 && cj[0] * cJm1[0] < 0 && cj[1] * cJm1[1] < 0))
		{
			if (bSeenOneTerminationCondition)
				break;
			bSeenOneTerminationCondition = true;
		}

		ThetaJm1 = thetaj;
		cJm1 = cj;
	}

	// Find indices of relevant sets

	double K2dStar = pParams->m_dK2 * dStar;
	double K3dStar = K3 * dStar;
	//Matrix<bool, -1, 1> D2Sel(nCtilde);
	//Matrix<bool, -1, 1> D3Sel(nCtilde);

	//D2Sel.setConstant(false);
	//D3Sel.setConstant(false);

	VectorXb* test2 = new VectorXb[nC];;
	VectorXb* test3 = new VectorXb[nC];

	Vector2d* temp = new Vector2d[nC];
	double* tempx = new double[nC];
	double* tempy = new double[nC];

	double* cdtrx = new double[nC];
	double* cdtry = new double[nC];

	//vector<VectorXb> test2, test3;
	//vector<VectorXd> diff;
	//test2.resize(nC);
	//test3.resize(nC);
	//diff.resize(nC);

	D_Delta_2_Ics.resize(0);
	Dtilde_Delta_2_Ics.resize(0);
	D_Delta_3_Ics.resize(0);
	Dtilde_Delta_3_Ics.resize(0);

	vector<bool> bTest2Any, bTest3Any;
	bTest2Any.resize(nC);
	bTest3Any.resize(nC);

	double cosj = cos(gj.theta);
	double sinj = sin(gj.theta);
	double dxj = gj.dx + zCM[0];
	double dyj = gj.dy + zCM[1];

	double K2dStarSquared = K2dStar * K2dStar;
	double K3dStarSquared = K3dStar * K3dStar;

#if 0
	FOR_START(c2, 0, nC)
		Vector2d temp = CDelta.row(c2).array() - zCM.array();
		double cdtrx = (temp(0) * cosj - temp(1) * sinj) + dxj;
		double cdtry = (temp(0) * sinj + temp(1) * cosj) + dyj;

		double* ctdx = (double*) CtildeDelta.data();		// remove const from data()
		double* ctdy = ctdx + nCtilde;
		test2[c2].resize(nCtilde);
		test3[c2].resize(nCtilde);
		bool bTest2any = false;
		bool bTest3any = false;

		for (int i = 0; i < nCtilde; i++)
		{
			double d = (ctdx[i] - cdtrx)*(ctdx[i] - cdtrx) + (ctdy[i] - cdtry)*(ctdy[i] - cdtry);
			/*bTest2any |= */(test2[c2][i] = d < K2dStarSquared);
			/*bTest3any |= */(test3[c2][i] = d < K3dStarSquared);
		}
		bTest2Any[c2] = test2[c2].any();
		bTest3Any[c2] = test3[c2].any();
	FOR_END
#else
	for (int c2 = 0; c2 < nC; c2++)
	{
		test2[c2].resize(nCtilde);
		test3[c2].resize(nCtilde);
	}

	double* pCDeltaX = (double*) &CDelta(0, 0);
	double* pCDeltaY = (double*) &CDelta(0, 1);
	double zCMx = zCM[0];
	double zCMy = zCM[1];

	double* ctdx = (double*)CtildeDelta.data();		// remove const from data()
	double* ctdy = ctdx + nCtilde;

//#pragma loop( hint_parallel( 0 ) )
//	for (int c2 = 0; c2 < nC; c2++)
//	{
//		tempx[c2] = pCDeltaX[c2] - zCMx;
//		double cdtrx = (tempx[c2] * cosj - tempy[c2] * sinj) + dxj;
//		double cdtry = (tempx[c2] * sinj + tempy[c2] * cosj) + dyj;
//
//		VectorXb& test2c2 = test2[c2];
//		VectorXb& test3c2 = test3[c2];
//	}

	//for (int c2 = 0; c2 < nC; c2++)
	//{
		FOR_START(c2, 0, nC)
		tempx[c2] = pCDeltaX[c2] - zCMx;
		tempy[c2] = pCDeltaY[c2] - zCMy;
		cdtrx[c2] = (tempx[c2] * cosj - tempy[c2] * sinj) + dxj;
		cdtry[c2] = (tempx[c2] * sinj + tempy[c2] * cosj) + dyj;

		FOR_END
	//}
#endif
		for (int c2 = 0; c2 < nC; c2++)
		{
			VectorXb& test2c2 = test2[c2];
			VectorXb& test3c2 = test3[c2];

			for (int i = 0; i < nCtilde; i++)
			{
				double d = (ctdx[i] - cdtrx[c2]) * (ctdx[i] - cdtrx[c2]) + (ctdy[i] - cdtry[c2]) * (ctdy[i] - cdtry[c2]);
				test2c2[i] = d < K2dStarSquared;
				test3c2[i] = d < K3dStarSquared;
			}
			bTest2Any[c2] = test2[c2].any();
			bTest3Any[c2] = test3[c2].any();
		}

		//DebugOutput("-----------------------------------------\n");

	for (int c2 = 0; c2 < nC; c2++)
	{
		if (bTest2Any[c2])
		{
			for (int i = 0; i < nCtilde; i++)
				if (test2[c2][i])
				{
					//DebugOutput("2: c2=%3d  i=%3d    diff=%.2f\n", c2, i, diff[c2][i]);
					Dtilde_Delta_2_Ics.push_back(i);
				}
			D_Delta_2_Ics.push_back(c2);
		}

		if (bTest3Any[c2])
		{
			for (int i = 0; i < nCtilde; i++)
				if (test3[c2][i])
				{
					//DebugOutput("3: c2=%3d  i=%3d    diff=%.2f\n", c2, i, diff[c2][i]);
					Dtilde_Delta_3_Ics.push_back(i);
				}
			D_Delta_3_Ics.push_back(c2);
		}
	}

	delete[] test2;
	delete[] test3;
	delete[] temp;
	delete[] tempx;
	delete[] tempy;
	delete[] cdtrx;
	delete[] cdtry;

	if (nC == 492)
		DebugOutput("Pre-Unique:  Dtilde_Delta_2_Ics has %d points, Dtilde_Delta_3_Ics has %d points\n", Dtilde_Delta_2_Ics.size(), Dtilde_Delta_3_Ics.size());


	auto icmp = [](const void* p1, const void* p2)
	{
		return *(int*)p1 - *(int*)p2;
	};
	qsort(Dtilde_Delta_2_Ics.data(), Dtilde_Delta_2_Ics.size(), sizeof(int), icmp);
	qsort(Dtilde_Delta_3_Ics.data(), Dtilde_Delta_3_Ics.size(), sizeof(int), icmp);

	auto ucmp = [](int i, int j)
	{
		return i == j;
	};
	auto it = unique(Dtilde_Delta_2_Ics.begin(), Dtilde_Delta_2_Ics.end(), ucmp);
	Dtilde_Delta_2_Ics.resize(std::distance(Dtilde_Delta_2_Ics.begin(), it));

	it = unique(Dtilde_Delta_3_Ics.begin(), Dtilde_Delta_3_Ics.end(), ucmp);
	Dtilde_Delta_3_Ics.resize(std::distance(Dtilde_Delta_3_Ics.begin(), it));

	if (nC == 492)
		DebugOutput("Post-Unique: Dtilde_Delta_2_Ics has %d points, Dtilde_Delta_3_Ics has %d points\n", Dtilde_Delta_2_Ics.size(), Dtilde_Delta_3_Ics.size());

	if (bPlot)
	{
		Progress.Delete(inith);
		if (debugh)
			Progress.Delete(debugh);

		if (debughtilde)
			Progress.Delete(debughtilde);

#ifdef _DEBUG
		LRESULT t2 = Progress.Plot(Gather(CtildeDelta, Dtilde_Delta_2_Ics), RGB(255, 255, 0), -5);
		LRESULT t3 = Progress.Plot(Gather(CtildeDelta, Dtilde_Delta_3_Ics), RGB(0, 255, 255), -5);
		//Sleep(500);
		Progress.Delete(t2);
		Progress.Delete(t3);
#endif
	}

	// Convert gj to the form used in SignatureSimilarity()

	//g_lock = [g_j(1), (g_j(2:3)' + z_cm' - [cos(g_j(1)), -sin(g_j(1)); sin(g_j(1)) cos(g_j(1))] * z_cm')'];

	c = cos(gj.theta);
	s = sin(gj.theta);
	rot << c, -s, s, c;

	translation = Vector2d(gj.dx, gj.dy) + zCMt - rot * zCMt;

	gLock = GTransform(gj.theta, translation(0), translation(1));
#ifdef _DEBUG
	//Sleep(100);
	int yyyy = 0;
#endif
}
