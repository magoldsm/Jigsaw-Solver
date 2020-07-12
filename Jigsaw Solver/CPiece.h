#pragma once

struct BivertexArc
{
	Curve	m_Contour;
	Curve	m_Signature;
};

class BATable : public Eigen::Matrix<BivertexArc, Eigen::Dynamic, 1>
{
public:
	void Serialize(CArchive& ar);
};

class CPiece
{
public:
	CPiece();
	~CPiece();

	void operator=(std::vector<cv::Point2d>& contour);

	Curve			m_Contour;
	Curve			m_Signature;
	double			m_Weight;
	int				m_nActive;
	BATable			m_Arcs;
	Eigen::VectorXi	m_Pt2Arc;

	void Serialize(CArchive& ar);

};

bool ReadPuzzle(std::vector<CPiece>& Pieces,  const char* filename);

