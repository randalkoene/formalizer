// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ header_description }}
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZGRAPHEDIT_ADDEDGE_HPP.
 */

#ifndef __FZGRAPHEDIT_ADDEDGE_HPP
#define __FZGRAPHEDIT_ADDEDGE_HPP (__VERSION_HPP)

// std
//#include <vector>

// core
//#include "config.hpp"
//#include "standard.hpp"
//#include "Graphbase.hpp"
#include "fzgraph.hpp"

using namespace fz;

Edge * add_Edge_request(Graph_modifications & gm, const Node_ID_key & depkey, const Node_ID_key & supkey, Edge_data & ed);

int make_edges();

#endif // __FZGRAPHEDIT_ADDEDGE_HPP
