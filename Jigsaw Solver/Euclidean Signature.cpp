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

	VectorXd /*xraw(sz), yraw(sz), */x(sz), y(sz), dx(sz), dy(sz), d2x(sz), d2y(sz), Area(sz), d1mag(sz), dkappa(sz);

//#define HOFF
	/*

b = sqrt(dx.^2+dy.^2);
a = circshift(b,[1 0]); % one behind
d = circshift(b,[-1 0]); % one ahead
g = circshift(b,[ 2 0]); % 2 behind
d2x = circshift(x,[-1 0]) - circshift(x,[1 0]);
d2y = circshift(y, [-1 0]) -circshift(y,[1 0]);

c = sqrt(d2x.^2+d2y.^2);

% Compute signed area using the cross product

va = [dx dy zeros(sz,1)];
vb = [bdx bdy zeros(sz,1)];
Area = .5*cross(va, vb);
Area = Area(:, 3);

% Compute approximate curvature
kappa = 4*(Area)./(a.*b.*c);

% Approximate arc length derivative of curvature
kappad = circshift(kappa,[-1 0]) - kappa;
kappad = (3/2)*((kappad)./(a+b+d) +  (circshift(kappad,[1 0]))./(a+b+g)); % Averaging 2 forward differences.

% Assign output
signature =  [kappa kappad];
*/

#ifdef HOFF
	VectorXd temp(sz), xm1(sz), ym1(sz), bdx(sz), bdy(sz), a(sz), b(sz), c(sz), d(sz), g(sz);

	bool bPlot = true;

	circShift(contour.x, xm1, -1);
	dx = xm1 - contour.x;
	circShift(contour.y, ym1, -1);
	dy = ym1 - contour.y;
	circShift(dx, bdx, 1);
	bdx = -bdx;
	circShift(dy, bdy, 1);
	bdy = -bdy;

	if (bPlot)
	{
		Plot("x", contour.x);
		Plot("y", contour.y);
		Plot("dx", dx);
		Plot("dy", dy);
		Plot("bdx", bdx);
		Plot("bdy", bdy);
	}

	b = (dx.array().square() + dy.array().square()).sqrt();
	circShift(b, a, 1);
	circShift(b, d, -1);
	circShift(b, g, 2);

	circShift(x, temp, 1);
	d2x = xm1 - temp;
	circShift(y, temp, 1);
	d2y = ym1 - temp;

	c = (d2x.array().square() + d2y.array().square()).sqrt();

	Area = 0.5*(dx.array() * bdy.array() - dy.array() * bdx.array());

	if (bPlot) Plot("Area", Area);

	sig.col(0) = 4 * Area.array() / (a.array()*b.array() * c.array());

	circShift(sig.col(0), temp, -1);
	sig.col(1) = temp - sig.col(0);
	circShift(sig.col(1), temp, 1);
	sig.col(1) = 1.5*(sig.col(1).array() / (a + b + c).array() + temp.array() / (a + b + g).array());

	if (bPlot) Plot("kappa", sig.col(0));
	if (bPlot) Plot("kappas", sig.col(1));

	waitKey(0);

#else
	//for (int i = 0; i < sz; i++)
	//{
	//	contour.row(0)[i] = (int) (rect.width*(1 + cos(i * 2 * 3.1415926 / sz)) / 2);
	//	contour.row(1)[i] = (int) (rect.height*(1 + sin(i * 2 * 3.1415926 / sz)) / 2);
	//}

	//xraw = contour.row(0);
	//yraw = contour.row(1);

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
