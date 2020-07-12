#pragma once

using EuclideanSignature = Curve;

class CPiece;

EuclideanSignature Orient_Reverse(EuclideanSignature& sig);

void MyNormm(Eigen::MatrixXd& mat, Eigen::Vector2d& dmin, Eigen::Vector2d& dmax);
void MyNormv(Eigen::VectorXd& vec, double& dmin, double& dmax);
void MyNormv(Eigen::VectorXd& vec);
void PlotContours(std::vector<Curve>& Curves, const char* windowName, bool bOverlay);
void Plot(const char* window, const Eigen::VectorXd& vec, double dDelta = 0.0, bool bAnimate = false);
void circShift(const Eigen::VectorXd& vin, Eigen::VectorXd& vout, int shift);
void circShift(const Curve& min, Curve& mout, int shift);
void MaxD(const std::vector<CPiece>& piece, Curve CPiece::*curve, double& width, double& height);
bool IsMember(int s, std::vector<int> v);
bool AnyMatch(const Eigen::VectorXi& v1, const std::vector<int>& v2);

Eigen::MatrixX2d Gather(const Eigen::MatrixX2d& src, std::vector<int> indices);
void AllExcept(std::vector<int>& dest, size_t size, std::vector<int> except);

CString CvtElapsedTime(long elapsed);

template<class T>
T RemoveElements(const  T& src, std::vector<int> remove)
{
	auto icmp = [](const void* p1, const void* p2) { return *(int*)p1 - *(int*)p2; };
	auto ucmp = [](int i, int j) { return i == j; };

	qsort(remove.data(), remove.size(), sizeof(int), icmp);
	auto it = unique(remove.begin(), remove.end(), ucmp);

	remove.resize(std::distance(remove.begin(), it));

	T res;
	size_t sz = remove.size();
	res.resize(src.rows() - sz, 2);
	int ires = 0;
	int iremove = 0;

	for (int i = 0; i < src.rows(); i++)
	{
		if (iremove >=sz || i < remove[iremove])
			res.row(ires++) = src.row(i);

		else if (i == remove[iremove])
			iremove++;
	}
	return res;
}


template<class T>
inline void Copy(Eigen::Matrix<T, -1, 1>& dest, const std::vector<T>& src)
{
	dest.resize(src.size());
	memcpy(dest.data(), src.data(), sizeof(T)*src.size());
}

template<class T>
void Append(T& dst, const T&src)
{
	T joined(dst.rows() + src.rows(), dst.cols());
	joined << dst, src;
	dst = joined;
}

template<class T, int r>
CArchive & operator<<(CArchive & ar, const Eigen::Matrix<T, r, 1>& m)
{
	ar << m.rows();
	for (int r = 0; r < m.rows(); r++)
		ar << m(r);
	return ar;
}

template<class T, int r>
CArchive & operator>>(CArchive & ar, Eigen::Matrix<T, r, 1>& m)
{
	size_t rows;
	ar >> rows;
	m.resize(rows, 1);
	for (int r = 0; r < rows; r++)
		ar >> m(r);
	return ar;
}

template<class T, int r, int c>
CArchive & operator<<(CArchive & ar, const Eigen::Matrix<T, r, c>& m)
{
	ar << m.rows();
	ar << m.cols();
	for (int r = 0; r < m.rows(); r++)
		for (int c = 0; c < m.cols(); c++)
			ar << m(r, c);
	return ar;
}


template<class T, int r, int c>
CArchive & operator>>(CArchive & ar, Eigen::Matrix<T, r, c>& m)
{
	size_t rows, cols;
	ar >> rows;
	ar >> cols;
	m.resize(rows, cols);
	for (int r = 0; r < rows; r++)
		for (int c = 0; c < cols; c++)
			ar >> m(r, c);
	return ar;
}

template<class T>
CArchive& operator<<(CArchive& ar, const std::vector<T>& v)
{
	ar << v.size();
	for (int i = 0; i < v.size(); i++)
		ar << v[i];
	return ar;
}


template<class T>
CArchive& operator>>(CArchive& ar, std::vector<T>& v)
{
	size_t sz;
	ar >> sz;
	v.resize(sz);

	for (int i = 0; i < v.size(); i++)
		ar >> v[i];
	return ar;
}

CArchive& operator<<(CArchive& ar, const char* x);
CArchive& operator>>(CArchive& ar, char*& x);

bool ApproxEqual(double d1, double d2);
void CheckArchiveLabel(CArchive& ar, const char* label);

extern CCriticalSection Pauser;
