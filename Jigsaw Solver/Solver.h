#pragma once

#include <iostream>
#include <atomic>
#include <fstream>
#include <cstdint>
#include <thread>


#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "Eigen/Dense"


#define USE_TBB
//#define USE_OMP

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/tick_count.h"

#ifdef USE_OMP
#include <omp.h>
#endif


using Curve = Eigen::Matrix<double, -1, 2>;

//class Curve : public Eigen::Matrix<double, -1, 2>
//{
//	size_t size() = delete;
//};
//

#define	PI			3.14159265358

#if defined(_DEBUG) && defined(WINVER)
void DebugOutput(const char* szFormat, ...);
#else
inline void DebugOutput(const char* szFormat, ...) {}
#endif

void AlwaysOutput(const char* szFormat, ...);






#ifdef USE_TBB
#define FOR_START(vbl, min, max)	tbb::parallel_for(min, max, [&](int vbl) {
#define	FOR_END						} );
#else
#ifdef USE_OMP
#define FOR_START(vbl, min, max)	_Pragma omp for \
									for (int vbl = min; vbl < max; vbl++) {
#define FOR_END						}
#else
#define FOR_START(vbl, min, max)	for (int vbl = min; vbl < max; vbl++) {
#define FOR_END						}
#endif
#endif

void Solver(const char* pszResume = nullptr);
