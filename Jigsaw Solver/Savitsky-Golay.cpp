#include "pch.h"
#include "Savitsy-Golay.h"

using namespace std;
using namespace Eigen;


static double GenFact(int a, int b)
{ // calculates the generalised factorial (a)(a-1)...(a-b+1)

	double gf = 1.0;

	for (int jj = (a - b + 1); jj < a + 1; jj++)
	{
		gf = gf * jj;
	}
	return (gf);

} // end of GenFact function



static double GramPoly(int i, int m, int k, int s)
{ // Calculates the Gram Polynomial ( s = 0 ), or its s'th
  // derivative evaluated at i, order k, over 2m + 1 points

	double gp_val;

	if (k > 0)
	{
//		gp_val = (4.0*k - 2.0) / (k*(2.0*m - k + 1))*(i*GramPoly(i, m, k - 1, s) + s * GramPoly(i, m, k - 1, s - 1)) - ((k - 1)*(2 * m + k)) / (k*(2 * m - k + 1))*GramPoly(i, m, k - 2, s);
		gp_val = (4.0*k - 2.0) / (k*(2.0*m - k + 1.0))*(i*GramPoly(i, m, k - 1, s) + s * GramPoly(i, m, k - 1.0, s - 1.0)) - ((k - 1.0)*(2.0*m + k)) / (k*(2.0*m - k + 1.0))*GramPoly(i, m, k - 2.0, s);
	}
	else
	{
		if ((k == 0) && (s == 0))
		{
			gp_val = 1.0;
		}
		else
		{
			gp_val = 0.0;
		} // end of if k = 0 & s = 0  
	} // end of if k > 0

	return (gp_val);

} // end of GramPoly function

double Weight(int i, int t, int m, int n, int s)
{ // calculates the weight of the i'th data point for the t'th Least-square
  // point of the s'th derivative, over 2m + 1 points, order n

	double sum = 0.0;

	for (int k = 0; k < n + 1; k++)
	{
//		sum += (2 * k + 1) * (GenFact(2 * m, k) / GenFact(2 * m + k + 1, k + 1)) * GramPoly(i, m, k, 0) * GramPoly(t, m, k, s);
		sum += (2.0*k + 1.0) * (GenFact(2.0*m, k) / GenFact(2.0*m + k + 1.0, k + 1.0)) * GramPoly(i, m, k, 0) * GramPoly(t, m, k, s);
	} // end of for loop

	return (sum);

} // end of Weight function

void GenerateSGVector(int deriv, int order, int window, VectorXd& sgvec)
{ // calculates the weight of the i'th data point (1st variable) for the t'th   Least-square point (2nd variable) of the s'th derivative (5th variable), over 2m + 1 points (3rd variable), order n (4th variable)

	sgvec.resize(2 * window + 1);
	char* done = new char[window + 3];
	memset(done, '0', window + 1);
	done[window + 1] = '\r';
	done[window + 2] = 0;

	tbb::parallel_for(0, window + 1, [&](int i)
		//		for (int i = 0; i <= window; i++)
	{
		//z = Weight(i, 0, window, order, deriv); // adjust input variables for required output
//#define SHOW_PROGRESS
		sgvec[i + window] = Weight(i, 0, window, order, deriv);;
		if (i != 0) sgvec[window - i] = Weight(-i, 0, window, order, deriv);
#ifdef SHOW_PROGRESS
		done[i] = '1';
		std::cout << done;
		std::cout.flush();
#endif
	});
#ifdef SHOW_PROGRESS
	std::cout << done << endl;
#endif
}

void
Convolve(const VectorXd& in, VectorXd& out, const VectorXd& filter)
{
	int sz = (int)in.rows();
	int sgWindow = (int)filter.size() / 2;

	FOR_START(i, 0, sz)
	//tbb::parallel_for(size_t(0), size_t(sz), [&](int i)
	//	//	for (int i = 0; i < sz; i++)
	//{
		if (i == 1931)
		{
			int q = 10;
		}
		out(i) = 0;
		for (int j = -sgWindow; j <= sgWindow; j++)
		{
			int jIdx = (i + j + sz) % sz;
			out(i) += in(jIdx) * filter(j + sgWindow);
		}
	//}
	//);
	FOR_END

	int i=0;
}


