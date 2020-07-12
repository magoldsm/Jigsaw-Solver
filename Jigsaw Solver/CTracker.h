#pragma once

class CTracker
{
	std::vector<int>		m_PlacedPieces;
	std::vector<int>		m_RemainingPieces;
	std::vector<int>		m_InactiveArcs;
	std::vector<int>		m_ActiveArcs;
	//std::vector<xxx>		m_InactivePoints;
	//std::vector<xxx>		m_ActivePoints;

};


class CPScore
{
public:
	CPScore(size_t sz) : m_Size(sz)
	{
		m_ArcScores = new Eigen::MatrixXd* [sz*sz];
	}

	~CPScore()
	{
		for (int i = 0; i < m_Size*m_Size; i++)
			if (m_ArcScores[i])
				delete m_ArcScores[i];
		delete[] m_ArcScores;
	}

	bool IsEmpty(size_t nRow, size_t nCol) { return m_ArcScores[nRow*m_Size + nCol] == NULL; }

	Eigen::MatrixXd& operator()(size_t nRow, size_t nCol)
	{
		return *(m_ArcScores[nRow*m_Size + nCol]);
	}

private:
	size_t		m_Size;
	Eigen::MatrixXd	**m_ArcScores;
};