#include "pch.h"

#include "CPiece.h"
#include "Utilities.h"

using namespace Eigen;
using namespace cv;
using namespace std;

CCriticalSection Pauser;

void MyNormm(Eigen::MatrixXd& mat, Vector2d& dmin, Vector2d& dmax)
{
	int imin, imax;

	dmin(0) = mat.row(0).minCoeff(&imin);
	dmin(1) = mat.row(1).minCoeff(&imin);
	dmax(0) = mat.row(0).maxCoeff(&imax);
	dmax(1) = mat.row(1).maxCoeff(&imax);
}

void MyNormv(VectorXd& vec, double& dmin, double& dmax)
{
	int imin, imax;

	dmin = vec.minCoeff(&imin);
	dmax = vec.maxCoeff(&imax);

	if (dmax != dmin) vec = (vec.array() - dmin) / (dmax - dmin);
}


void MyNormv(VectorXd& vec)
{
	double dmin;
	double dmax;

	MyNormv(vec, dmin, dmax);
}


void
PlotContours(vector<Curve>& Curves, const char* windowName, bool bOverlay)
{
	vector<Curve> Norms = Curves;
	size_t sz = Curves.size();
	vector<double> dKappaMin(sz), dKappaMax(sz);
	vector<double> dKappasMin(sz), dKappasMax(sz);

	for (int i = 0; i < sz; i++)
	{
		Curve& norm = Norms[i];
		VectorXd normkappa = norm.col(0);
		VectorXd normkappas = norm.col(1);
		MyNormv(normkappa, dKappaMin[i], dKappaMax[i]);
		MyNormv(normkappas, dKappasMin[i], dKappasMax[i]);
		norm.col(0) = normkappa;
		norm.col(1) = normkappas;
	}

	//int key;

	vector<Mat> drawings;
	vector<char*> titles;
	drawings.resize(sz);
	titles.resize(sz);

	for (int i = 0; i < sz; i++) {
		drawings[i] = Mat::zeros(512, 512, CV_8UC1);
		titles[i] = new char[100];
		sprintf_s(titles[i], 100, "%s%d", windowName, i);
	}
	
	//do
	//{
		for (int i = 0; i < sz; i++)
		{
			int dIndex = bOverlay ? 0 : i;
			Curve& norm = Norms[i];
			for (int j = 0; j < norm.rows(); j++)
			{
				int X = (int)(norm(j, 0) * 511);
				int Y = (int)(norm(j, 1) * 511);
				if (X < 0) X = 0;
				if (Y < 0) Y = 0;
				drawings[dIndex].at<unsigned char>(X, Y) = 255;
				imshow(titles[dIndex], drawings[dIndex]);
			}
		}
	//	key = waitKey(0);
	//} while (key != 27);

	for (int i = 0; i < sz; i++) {
		delete [] titles[i];
	}

}

void circShift(const Curve& min, Curve& mout, int shift)
{
	mout.resizeLike(min);

	int sz = (int)min.rows();

	if (shift < 0)
	{
		mout.bottomRows(sz + shift) = min.topRows(sz + shift);
		mout.topRows(-shift) = min.bottomRows(-shift);
		return;
	}
	else
	{
		mout.topRows(sz - shift) = min.bottomRows(sz - shift);
		mout.bottomRows(shift) = min.topRows(shift);
		return;
	}
}


void circShift(const VectorXd& vin, VectorXd& vout, int shift)
{
	vout = vin;

	int sz = (int)vin.size();

	if (shift < 0)
	{
		vout.segment(-shift, sz + shift) = vin.segment(0, sz + shift);
		vout.segment(0, -shift) = vin.segment(sz + shift, -shift);
		return;
	}
	if (shift > 0)
	{
		vout.segment(0, sz - shift) = vin.segment(shift, sz - shift);
		vout.segment(sz - shift, shift) = vin.segment(0, shift);
		return;
	}
}

// Computes the bounding box that would hold all of the curves of all pieces.
// Since pieces can hold multiple curves, the "curve" parameter is a pointer-to-member
// that specifies which curve to analyze

void MaxD(const std::vector<CPiece>& pieces, Curve CPiece::*curve, double& width, double& height)
{
	size_t sz = pieces.size();

	width = 0;
	height = 0;

	for (int i = 0; i < sz; i++)
	{
		const CPiece& piece = pieces[i];

		Curve c = piece.*curve;
		double w = c.col(0).maxCoeff() - c.col(0).minCoeff();
		double h = c.col(1).maxCoeff() - c.col(1).minCoeff();
		// cout << i << " " << w << " " << h << endl;
		width = max(width, w);
		height = max(height, h);
	}

	if (height == 0)
		height = 1;
	if (width == 0)
		width = 1;
}

bool IsMember(int s, const std::vector<int> v)
{
	for (int i = 0; i < v.size(); i++)
		if (s == v[i])
			return true;
	return false;
}

bool AnyMatch(const VectorXi & v1, const std::vector<int> & v2)
{
	for (int i = 0; i < v1.size(); i++)
		for (int j = 0; j < v2.size(); j++)
			if (v1[i] == v2[j])
				return true;
	return false;
}

// Construct a new MatrixX2d that is a subset of "src" using the indices

Eigen::MatrixX2d Gather(const MatrixX2d & src, std::vector<int> indices)
{
	size_t sz = indices.size();
	MatrixX2d res(sz, 2);

	for (int i = 0; i < sz; i++)
		res.row(i) = src.row(indices[i]);

	return res;
}

void
Plot(const char* window, const VectorXd& vec, double dDelta, bool bAnimate)
{
	VectorXd norm = vec;
	int sz = (int)vec.size();

	double min, max;

	MyNormv(norm, min, max);
	
	Mat drawing = Mat::zeros(Size(512, 512), CV_8UC1);

	for (int i = 0; i < sz; i++)
	{
		int x = 512 * i / sz;
		int y = 511 - (int)(norm(i) * 511);
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		drawing.at<unsigned char>(y, x) = 255;
		if (bAnimate) {
			imshow(window, drawing);
			waitKey(1);
		}
	}

	if (dDelta > 0.0)
	{
		double dDeltaPlus = (dDelta - min) / (max - min);
		double dDeltaMinus = (-dDelta - min) / (max - min);
		line(drawing, Point(0, (int)(dDeltaPlus * 512)), Point(511, (int)(dDeltaPlus * 512)), Scalar(255, 255, 255), 1);
		line(drawing, Point(0, (int)(dDeltaMinus * 512)), Point(511, (int)(dDeltaMinus * 512)), Scalar(255, 255, 255), 1);
	}

	char buff[1000];
	sprintf_s(buff, sizeof(buff), "%f", min);
	cv::putText(drawing, buff, Point(10, 500), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
	sprintf_s(buff, sizeof(buff), "%f", max);
	cv::putText(drawing, buff, Point(10, 25), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);

	imshow(window, drawing);
}



// Reverses a Euclidean Signature.  Negates kappa and reverses the order of both
EuclideanSignature
Orient_Reverse(EuclideanSignature& sig)
{
	EuclideanSignature newsig;
	newsig.resizeLike(sig);

	newsig.col(0) = -sig.col(0).reverse();
	newsig.col(1) = sig.col(1).reverse();

	return newsig;
}


// Generates an array of 0..size-1 and then removes elements listed in the "except" vector

void
AllExcept(vector<int>& dest, size_t size, vector<int> except)
{
	dest.resize(size);
	for (int i = 0; i < size; i++)
		dest[i] = i;

	auto icmp = [](const void* p1, const void* p2) { return *(int*)p1 - *(int*)p2; };
	auto ucmp = [](int i, int j) { return i == j; };

	qsort(except.data(), except.size(), sizeof(int), icmp);
	auto it = unique(except.begin(), except.end(), ucmp);

	except.resize(std::distance(except.begin(), it));

	for (int i = (int) except.size() - 1; i >= 0; i--)
		if (except[i] != -1)
			dest.erase(dest.begin() + except[i]);
}

CString CvtElapsedTime(long elapsed)
{
	CString str;

	if (elapsed < 60)
	{
		str.Format(_T("%d Seconds"), elapsed);
	}
	else
	{
		long seconds = elapsed % 60;
		long minutes = elapsed / 60;
		if (minutes < 60)
		{
			str.Format(_T("%d Min %d Sec"), minutes, seconds);
		}
		else
		{
			minutes %= 60;
			long hours = elapsed / 3600;
			if (hours < 24)
			{
				str.Format(_T("%d H  %d M  %d S"), hours, minutes, seconds);
			}
			else
			{
				long days = elapsed / (3600 * 24);
				hours %= 24;
				str.Format(_T("%d D  %d H  %d M  %d S"), days, hours, minutes, seconds);
			}
		}
	}

	return str;
}


void CheckArchiveLabel(CArchive& ar, const char* label)
{
	char* pszLabel;
	ar >> pszLabel;

	ASSERT(strcmp(pszLabel, label) == 0);
}


CArchive& operator<<(CArchive& ar, const char* x)
{
	ar << strlen(x);
	ar.Write(x, (UINT)(strlen(x) + 1));
	return ar;
}

CArchive& operator>>(CArchive& ar, char*& x)
{
	size_t sz;
	ar >> sz;
	x = new char[sz + 1];
	ar.Read(x, (UINT) sz+1);
	return ar;
}

bool ApproxEqual(double d1, double d2)
{
	return (fabs(d1 - d2) < fabs(d1) / 1000.0);
}

