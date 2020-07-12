#pragma once

using Indices = std::vector<int>;

void Lock(CFit& fit, const Curve& CDelta, const Curve& CtildeDelta, GTransform& gLock, double K3, Indices& D_Delta_3_Ics, Indices& Dtilde_Delta_3_Ics, Indices& D_Delta_2_Ics, Indices& Dtilde_Delta_2_Ics, bool bPlot, LRESULT& plotHandle);
