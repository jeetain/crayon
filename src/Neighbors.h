//
// Neighbors.h
// wraps the voro++ library for calculation of neighbor lists
//
// Copyright (c) 2018 Wesley Reinhart.
// This file is part of the crayon project, released under the Modified BSD License.

#ifndef SRC_NEIGHBORS_H_
#define SRC_NEIGHBORS_H_

#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>

#include <Eigen/Core>

#include <voro++/voro++.hh>

namespace crayon
{

std::vector<std::vector<int>> VoroNeighbors(const Eigen::MatrixXf &R, const Eigen::VectorXf &L);

void export_VoroNeighbors(pybind11::module& m);

} // end namespace crayon

#endif // SRC_NEIGHBORS_H_
