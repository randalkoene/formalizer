// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ header_description }}
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZGRAPHEDIT_ADDNODE_HPP.
 */

#ifndef __FZGRAPHEDIT_ADDNODE_HPP
#define __FZGRAPHEDIT_ADDNODE_HPP (__VERSION_HPP)

// std
//#include <vector>

// core
//#include "config.hpp"
//#include "standard.hpp"
//#include "Graphbase.hpp"
#include "fzgraphedit.hpp"

using namespace fz;

Node * add_Node_request(Graph_modifications & gm, Node_data & nd);

#endif // __FZGRAPHEDIT_ADDNODE_HPP
