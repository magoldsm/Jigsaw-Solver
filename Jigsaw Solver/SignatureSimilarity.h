#pragma once

class CTracker
{
public:
	CTracker() {}

	void SetSize(size_t sz)
	{
		m_PlacedPieces.resize(sz);
		m_RemainingPieces.resize(sz);
		m_InactiveArcs.resize(sz);
		m_ActiveArcs.resize(sz);
		m_InactivePoints.resize(sz);
		m_ActivePoints.resize(sz);
		m_Pc2Place.resize(sz);
	}
	std::vector<int>				m_PlacedPieces;
	std::vector<int>				m_RemainingPieces;
	std::vector<std::vector<int> >	m_InactiveArcs;
	std::vector<std::vector<int>>	m_ActiveArcs;
	std::vector<std::vector<int> >	m_InactivePoints;
	std::vector<std::vector<int> >	m_ActivePoints;
	std::vector<int>				m_Pc2Place;
	Curve							m_SolvedPuzzleBoundary;
	Eigen::Matrix<int, -1, 2>		m_SPB_Pt2PcPt;
	std::vector<int>				m_SP_Bdry_PtIcs_3;
	std::vector<int>				m_SP_Bdry_PtIcs;

	void Serialize(CArchive& ar);
};



struct GTransform
{
	GTransform() { theta = 0.0; dx = 0.0; dy = 0.0; }
	GTransform(double _theta, double _dx, double _dy) { theta = _theta; dx = _dx; dy = _dy; }
	double		theta;
	double		dx;
	double		dy;
	void Serialize(CArchive& ar);
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

	void Serialize(CArchive& ar);
	void Dump(int nIdx, std::ofstream& file);

	static void Dump(const char* pszFilename);
};


class CPlacement
{
public:
	CPlacement(int nPiece, const Eigen::Vector4d& Score, const GTransform& gLock, const CFit& Fit)
		: m_nPiece(nPiece)
		, m_Score(std::move(Score))
		, m_gLock(std::move(gLock))
		, m_Fit(std::move(Fit))
	{
		int x = 0;
	}
	CPlacement(CArchive& ar);
	int								m_nPiece;
	Eigen::Vector4d					m_Score;
	GTransform						m_gLock;
	CFit							m_Fit;
	std::vector<int>				m_Neighbors;

	void Serialize(CArchive& ar);

private:
	
};

void PlacePieces();


class CPScore
{
public:
	CPScore(size_t sz);
	CPScore() : m_Size(0), m_ArcScores(NULL) {}
	~CPScore();

	void SetSize(size_t sz);
	size_t GetSize() { return m_Size; }

	bool IsEmpty(size_t nRow, size_t nCol)
	{
		return m_ArcScores[nRow*m_Size + nCol].size() == 0;
	}

	Eigen::MatrixXd& operator()(size_t nRow, size_t nCol)
	{
		return (m_ArcScores[nRow*m_Size + nCol]);
	}

	void Display(double dP0 = 1.0);
	void Display(size_t nRow, size_t nCol, double dP0 = 1.0);

	void Serialize(CArchive& ar);

private:
	size_t				m_Size;
	Eigen::MatrixXd*	m_ArcScores;
};

Curve TransformCurve(const Curve& points, const GTransform& trans);

extern std::vector<CPlacement> Placements;
extern std::vector<CTracker> Tracker;
extern CPScore PScores;
extern std::vector<CFit> Fits;
