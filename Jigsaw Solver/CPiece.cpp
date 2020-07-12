#include "pch.h"
#include <iostream>
#include <fstream>
#include "CPiece.h"
#include "Utilities.h"

using namespace std;
using namespace cv;
using namespace Eigen;

CPiece::CPiece()
{
}


CPiece::~CPiece()
{
}

static char *
ParsePiece(vector<Point2d>& piece, char* cp)
{
	char* endptr;

	double x = strtof(cp, &endptr);
	cp = endptr + 1;
	double y = strtof(cp, &endptr);
	
//	std::cout << x << y ;

	if (x < 1000 && x >= 0)
	{
//		cout << " push";
		piece.push_back(Point2d(x, y));
	}
	else
	{
		int q = 0;
	}
//	cout << endl;
	return endptr+1;
}

static bool
ParseLine(vector<vector<Point2d> >& Pieces, char* buff)
{
	char* cp = buff;
	for (int i = 0; i < Pieces.size(); i++)
	{
		cp = ParsePiece(Pieces[i], cp);
	}
	return false;
}


bool ReadPuzzle(std::vector<CPiece>& Pieces, const char* filename)
{
	char buff[5000];

	ifstream file;
	file.open(filename, ios::in);

	if (!file.is_open())
		return false;

	file.getline(buff, sizeof(buff));

	int nCommas = 0;
	char* cp = strchr(buff, ',');

	while (cp)
	{
		nCommas++;
		cp = strchr(cp + 1, ',');
	}

	if (nCommas == 1) nCommas++;

	//if ((nCommas + 1) % 3 != 0)
	//{
	//	cout << "Wrong number of fields\n";
	//	return false;
	//}

	int nPieces = (nCommas + 1) / 2;
	Pieces.resize(nPieces);
	vector<vector<Point2d> > tempPieces;
	tempPieces.resize(nPieces);

	while (!file.fail())
	{
		file.getline(buff, sizeof(buff));
//		cout << buff << endl;
		//if (file.eof())
		//	break;

		if (strlen(buff))						// Skip blank lines
			ParseLine(tempPieces, buff);
	}

	for (int i = 0; i < nPieces; i++)
	{
		Pieces[i] = tempPieces[i];
	}
	
	return true;
}

// Assigning an OpenCV contour to a CPiece initializes the piece's contour

void CPiece::operator=(std::vector<cv::Point2d>& contour)
{
	size_t sz = contour.size();

	m_Contour.resize(sz, 2);

	for (size_t i = 0; i < sz; i++)
	{
		m_Contour(i, 0) = contour[i].x;
		m_Contour(i, 1) = contour[i].y;
	}
}

void CPiece::Serialize(CArchive & ar)
{
	if (ar.IsStoring())
	{
		ar << "Piece";

		ar << m_Contour;
		ar << m_Signature;
		ar << m_Weight;
		ar << m_nActive;
		m_Arcs.Serialize(ar);
		ar << m_Pt2Arc;
	}
	else
	{
		CheckArchiveLabel(ar, "Piece");

		ar >> m_Contour;
		ar >> m_Signature;
		ar >> m_Weight;
		ar >> m_nActive;
		m_Arcs.Serialize(ar);
		ar >> m_Pt2Arc;
	}
}

void BATable::Serialize(CArchive & ar)
{
	if (ar.IsStoring())
	{
		ar << rows();
		for (int r = 0; r < rows(); r++)
		{
			ar << (*this)(r).m_Contour;
			ar << (*this)(r).m_Signature;
		}
	}
	else
	{
		size_t rows;
		ar >> rows;
		this->resize(rows, 1);
		for (int r = 0; r < rows; r++)
		{
			ar >> (*this)(r).m_Contour;
			ar >> (*this)(r).m_Signature;
		}
	}
}
