#include "pch.h"
#include "CProgress.h"


void CProgress::RestartReport(int n, bool bStart)
{
	m_Bars[n].Start(bStart);
}

void CProgress::RestartReport(int n, int m, bool bStart)
{
	m_Bars[n].m_Bars[m].Start(bStart);
}

void CProgressBar::Start(bool bStart)
{
	if (bStart)
		QueryPerformanceCounter(&m_Time);
	else
		m_Time.QuadPart = 0;
}
