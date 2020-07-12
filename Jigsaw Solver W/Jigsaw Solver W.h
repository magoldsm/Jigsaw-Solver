
// Jigsaw Solver W.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CJigsawSolverWApp:
// See Jigsaw Solver W.cpp for the implementation of this class
//

class CJigsawSolverWApp : public CWinApp
{
public:
	CJigsawSolverWApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CJigsawSolverWApp theApp;

#include "CPiece.h"

extern std::vector<CPiece> Pieces;
extern Eigen::VectorXd Weights;
extern double Dx, Dy, Dkappa, Dkappas;

