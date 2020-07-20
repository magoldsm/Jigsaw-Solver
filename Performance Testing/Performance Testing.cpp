// Performance Testing.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Eigen/Dense"
#include <vector>

struct GTransform
{
	GTransform() { theta = 0.0; dx = 0.0; dy = 0.0; }
	GTransform(double _theta, double _dx, double _dy) { theta = _theta; dx = _dx; dy = _dy; }
	double		theta;
	double		dx;
	double		dy;
};


class CFit
{
public:
	CFit() : m_Size(0), m_Slot(0), m_Score(-1), m_Pieces(-1, -1) {}

	void MeanOfAngles();								// Go through m_ArcTrans, eliminate outliers and compute m_gFit.theta

	Eigen::Vector2i					m_Pieces;
	int								m_Size;
	Eigen::MatrixX2i				m_Arcs;				// Rows given by m_Size
	std::vector<GTransform>			m_ArcTrans;			// Rows given by m_Size
	GTransform						m_gFit;
	double							m_Score;
	int								m_Slot;

	void Dump(int nIdx, std::ofstream& file);

	static void Dump(const char* pszFilename);
};

int main()
{
	Eigen::Vector2i a, b;

	a = b;

	a = std::move(b);

	GTransform ga, gb;

	ga = gb;

	ga = std::move(gb);
}


