//
// Neighborhood.cc
// wraps the libgraphlet/Orca calculation of GDVs and GDDs
//
// Copyright (c) 2018 Wesley Reinhart.
// This file is part of the crayon project, released under the Modified BSD License.

#include "Neighborhood.h"

namespace crayon
{

Neighborhood::Neighborhood()
    {
    }

Neighborhood::Neighborhood(const Eigen::MatrixXi &A)
    : A_(A)
    {
    buildFromAdj();
    setup();
    }

Neighborhood::Neighborhood(const Eigen::MatrixXi &A, const int k)
    : A_(A), k_(k)
    {
    buildFromAdj();
    setup();
    }

Neighborhood::Neighborhood(const Graph &G)
    : G_(G)
    {
    setup();
    }

Neighborhood::Neighborhood(const Graph &G, const int k)
    : G_(G), k_(k)
    {
    setup();
    }

Neighborhood::~Neighborhood()
    {
    }

void Neighborhood::buildFromAdj()
    {
    // setup graph instance
    std::map<int,Graph::vertex_descriptor> map;
    Graph::vertex_descriptor n = A_.rows();
    G_ = Graph(n);
    G_[boost::graph_bundle].label = "AdjMat";
    // fill nodes first
    for( unsigned int i = 0; i < n; i++ )
        {
        map[i] = i;
        Graph::vertex_descriptor v = map[i];
        G_[v].label = std::to_string(i + 1);
        }
    // now loop through nodes and fill edges
    for( unsigned int i = 0; i < n; i++ )
        {
        for( unsigned int j = 0; j < n; j++ )
            {
            if( A_(i,j) != 0 || A_(j,i) != 0 )
                {
                Graph::vertex_descriptor u = map[i];
                Graph::vertex_descriptor v = map[j];
                auto e = add_edge(u, v, G_);
                G_[e.first].label = std::to_string(i+1) + "-" + std::to_string(j+1);
                }
            }
        }
    }

void Neighborhood::setup()
    {
    // clean up graph and initialize orca object
    remove_edge_loops(G_);
    std::vector<std::pair<size_t,size_t>> edges;
    get_edges(G_, edges);
    O_.reset(new orca::Orca(num_vertices(G_), edges, k_));
    O_->compute();
    }

Eigen::MatrixXi Neighborhood::getGDV()
    {
    if( !computed_gdv_ ) computeGDV();
    return GDV_;
    }

void Neighborhood::computeGDV()
    {
    // compute gdv with orca and convert to numpy-readable vector
    GDV_ = Eigen::MatrixXi::Zero(num_vertices(G_), orca::ORBITS[k_]);
    const boost::numeric::ublas::matrix<int64_t> orbits = O_->getOrbits();
    for( unsigned int i = 0; i < num_vertices(G_); i++ )
        {
        for( unsigned int j = 0; j < orca::ORBITS[k_]; j++ )
            {
            GDV_(i,j) = int(orbits(i,j));
            }
        }
    computed_gdv_ = true;
    }

Eigen::MatrixXi Neighborhood::getGDD()
    {
    if( !computed_gdd_ ) computeGDD();
    return GDD_;
    }

void Neighborhood::computeGDD()
    {
    // compute gdd
    libgraphlet::GDD gdd;
    libgraphlet::gdd(*O_, gdd, false);
    // evaluate size of each element
    size_t m = 0;
    size_t n = gdd.size();
    for(size_t i = 0; i < n; ++i)
        {
        size_t o = (gdd[i].size() > 0 ? gdd[i].rbegin()->first : 0);
        if( o > m ) m = o;
        }
    // allocate matrix
    GDD_ = Eigen::MatrixXi::Zero(n,m+1);
    // populate matrix
    for(size_t i = 0; i < n; ++i)
        {
        m = (gdd[i].size() > 0 ? gdd[i].rbegin()->first : 0);
        for(size_t j = 1; j <= m; ++j)
            {
            if(gdd[i].find(j) != gdd[i].end())
                {
                GDD_(i,j) = (int) gdd[i].at(j);
                }
            }
        }
    computed_gdd_ = true;
    }

void export_Neighborhood(pybind11::module& m)
    {
    pybind11::class_<Graph>(m,"graph")
        .def(pybind11::init<const unsigned int>());
    pybind11::class_<Neighborhood>(m,"neighborhood")
        .def(pybind11::init<const Eigen::MatrixXi &>())
        .def(pybind11::init<const Eigen::MatrixXi &, const int>())
        .def(pybind11::init<const Graph &>())
        .def(pybind11::init<const Graph &, const int>())
        .def("adj", &Neighborhood::getAdj)
        .def("gdv", &Neighborhood::getGDV)
        .def("gdd", &Neighborhood::getGDD)
    ;
    }

}  // end namespace crayon
