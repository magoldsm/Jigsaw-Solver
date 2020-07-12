// RotateData.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;


#define	N_PIECES		1000
#define	N_POINTS		2000

double Pieces[N_PIECES][N_POINTS][2];			// 1000 pieces, 2000 points each
double OnePiece[N_POINTS * 2];

char buff[1024 * 1024];

int main(int argc, char** argv)
{
	int nLargest = 0;

	ifstream file;
	file.open(argv[1], ios::in);

	if (!file.is_open())
		return false;

	int nPieces;
	for (nPieces = 0; nPieces < N_PIECES; nPieces++)
	{
		if (!file)
			break;

		file.getline(buff, sizeof(buff));

		int j;
		char* cp = buff;

		for (j = 0; j < N_POINTS; j++)
		{
			stringstream str(cp);
			str >> OnePiece[j];
			char* pComma = strchr(cp, ',');
			if (!pComma)
				break;
			cp = pComma + 1;
			if (*cp == ',')
				break;
		}
		//stringstream str(buff);

		//int j = 0;

		//while (str >> OnePiece[j++])
		//	;

		j = (j + 1) / 2;

		for (int k = 0; k < j; k++)
		{
			Pieces[nPieces][k][0] = OnePiece[k];
			Pieces[nPieces][k][1] = OnePiece[k + j];
		}

		if (j > nLargest)
			nLargest = j;

		while (j < N_POINTS)
		{
			Pieces[nPieces][j][0] = Pieces[nPieces][j][1] = 999999.0;
			j++;
		}
	}
	nPieces--;
	file.close();

	ofstream ofile;
	ofile.open("rotated.csv", std::ios::out | std::ios::trunc);

	for (int y = 0; y < nPieces; y++)
		ofile << y << ",,";
	ofile << endl;

	for (int x = 0; x < nLargest; x++)
	{
		for (int y = 0; y < nPieces; y++)
		{
			if (y)
				ofile << ",";
			ofile << Pieces[y][x][0] << "," << Pieces[y][x][1];
		}
		ofile << endl;
	}

	ofile.close();

	return 0;

}

