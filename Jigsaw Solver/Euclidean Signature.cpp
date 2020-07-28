#include "pch.h"
#include "Euclidean Signature.h"
#include "Savitsy-Golay.h"
#include "Utilities.h"
#include "CPiece.h"

using namespace std;
using namespace cv;
using namespace Eigen;


void
CalcEuclideanSignature(CPiece& piece, VectorXd smoothVec, VectorXd d1Vec, VectorXd d2Vec)
{
	Curve& contour = piece.m_Contour;
	EuclideanSignature& sig = piece.m_Signature;

	int sz = (int)contour.rows();
	sig.resize(sz, 2);

#define HOFF

#ifdef HOFF
	VectorXd dx(sz), dy(sz), bdx(sz), bdy(sz), Area(sz), dkappa(sz);

	dx = circShift(contour.col(0), -1) - contour.col(0);							// dx = circshift(x,[-1 0]) - x; % Forward difference
	dy = circShift(contour.col(1), -1) - contour.col(1);							// dy = circshift(x,[-1 0]) - y; % Forward difference
	bdx = -circShift(dx, 1);														// bdx = -circshift(dx, [1 0]);  % Backward difference
	bdy = -circShift(dy, 1);														// bdy = -circshift(dy, [1 0]);  % Backward difference

	VectorXd b = (dx.array().square() + dy.array().square()).sqrt();				// b = sqrt(dx. ^ 2 + dy. ^ 2);
	VectorXd a = circShift(b, 1);													// a = circshift(b, [1 0]);      % one behind
	VectorXd d = circShift(b, -1);													// d = circshift(b, [-1 0]);     % one ahead
	VectorXd g = circShift(b, 2);													// g = circshift(b, [2 0]);      % 2 behind
	VectorXd d2x = circShift(contour.col(0), -1) - circShift(contour.col(0), 1);	// d2x = circshift(x, [-1 0]) - circshift(x, [1 0]);
	VectorXd d2y = circShift(contour.col(1), -1) - circShift(contour.col(1), 1);	// d2y = circshift(y, [-1 0]) - circshift(y, [1 0]);

	VectorXd c = (d2x.array().square() + d2y.array().square()).sqrt();				// c = sqrt(d2x. ^ 2 + d2y. ^ 2);

	// % Compute signed area using the cross product

	Area = 0.5 * (dx.array() * bdy.array() - dy.array() * bdx.array());				// va = [dx dy zeros(sz, 1)];
																					// vb = [bdx bdy zeros(sz, 1)];
																					// Area = .5 * cross(va, vb);
																					// Area = Area(:, 3);

	// % Compute approximate curvature

	sig.col(0) = -4.0 * Area.array() / (a.array() * b.array() * c.array());			// kappa = 4 * (Area). / (a.*b.*c);

	// % Approximate arc length derivative of curvature

	dkappa = sig.col(0) - circShift(sig.col(0), -1);								// kappad = circshift(kappa, [-1 0]) - kappa;

	// kappad = (3 / 2) * ((kappad). / (a + b + d) + (circshift(kappad, [1 0])). / (a + b + g));% Averaging 2 forward differences.
	sig.col(1) = 1.5 * (dkappa.array() / (a.array() + b.array() + d.array()) + circShift(dkappa, 1).array() / (a.array() + b.array() + g.array()));
		


#else
	VectorXd x(sz), y(sz), dx(sz), dy(sz), d2x(sz), d2y(sz), Area(sz), d1mag(sz), dkappa(sz);

	// Smooth the contour to overcome the effects of digitization.

	Convolve(contour.col(0), x, smoothVec);
	Convolve(contour.col(1), y, smoothVec);

	// Take 1st and 2nd derivatives

	Convolve(contour.col(0), dx, d1Vec);
	Convolve(contour.col(1), dy, d1Vec);

	Convolve(contour.col(0), d2x, d2Vec);
	Convolve(contour.col(1), d2y, d2Vec);

	bool bPlot = false;

	if (bPlot) Plot("x", x);
	if (bPlot) Plot("y", y);
	if (bPlot) Plot("dx", dx);
	if (bPlot) Plot("dy", dy);
	if (bPlot) Plot("d2x", d2x);
	if (bPlot) Plot("d2y", d2y);

	Area = (dx.array() * d2y.array() - dy.array()*d2x.array());

	if (bPlot) Plot("Area", Area);

	d1mag = (dx.array().square() + dy.array().square()).cube().sqrt();
	sig.col(0) = Area.array() / d1mag.array();

	Convolve(sig.col(0), dkappa, d1Vec);
	sig.col(1) = dkappa;

	if (bPlot) Plot("kappa", sig.col(0));
	if (bPlot) Plot("kappas", sig.col(1));

	//cout << "x|y|dx|dy|d2x|d2y|Area|kappa|kappas\n";

	//for (int i = 0; i < sz; i++)
	//{
	//	cout << contour.x[i] << "|" << contour.y[i] << "|" << dx[i] << "|" << dy[i] << "|" << d2x[i] << "|" << d2y[i] << "|" << Area[i] << "|" << sig.col(0)[i] << "|" << sig.col(1)[i] << endl;
	//}

	// ***BUGBUG***  Hoff's MATLAB program does this, but I'm not sure why

	//if (sig.col(0).sum() < 0)
	//{
	//	// H&O also flipud the piece (in our case, contour).

	//	Orient_Reverse(sig);
	//}

#endif
}
