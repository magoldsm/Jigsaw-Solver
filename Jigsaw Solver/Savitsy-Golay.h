#pragma once

void GenerateSGVector(int deriv, int order, int window, Eigen::VectorXd& sgvec);
void Convolve(const Eigen::VectorXd& in, Eigen::VectorXd& out, const Eigen::VectorXd& filter);
