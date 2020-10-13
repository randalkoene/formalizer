// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares basic information gathering functions for use with
 * Graph data structures.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __GRAPHINFO_HPP.
 */

#ifndef __GRAPHINFO_HPP
#include "coreversion.hpp"
#define __GRAPHINFO_HPP (__COREVERSION_HPP)

// std

// core
#include "Graphtypes.hpp"

namespace fz {

std::string Graph_Info(Graph & graph);

std::string List_Topics(Graph & graph, std::string delim);

struct Nodes_Stats {
    unsigned long num_completed = 0; ///< marked completion >= 1.0
    unsigned long num_open = 0;      ///< marked 0.0 <= completion < 1.0
    unsigned long num_other = 0;     ///< marked completion < 0.0
    unsigned long sum_required_completed = 0;
    unsigned long sum_required_open = 0;
};

Nodes_Stats Nodes_statistics(Graph & graph);

std::string Nodes_statistics_string(const Nodes_Stats & nstats);

unsigned long Edges_with_data(Graph & graph);

} // namespace fz

#endif // __GRAPHINFO_HPP
