// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ header_description }}
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZGRAPH_NAMEDNODELIST_HPP.
 */

#ifndef __FZGRAPH_NAMEDNODELIST_HPP
#define __FZGRAPH_NAMEDNODELIST_HPP (__VERSION_HPP)

// std
//#include <vector>

// core
//#include "config.hpp"
//#include "standard.hpp"
//#include "Graphbase.hpp"
//#include "fzgraph.hpp"

using namespace fz;

// forward declarations
#ifndef __GRAPHMODIFY_HPP
class Graph_modifications;
#endif

int add_to_list();

int remove_from_list();

int delete_list();

#endif // __FZGRAPH_NAMEDNODELIST_HPP
