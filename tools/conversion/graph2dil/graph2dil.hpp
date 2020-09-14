// Copyright 2020 Randal A. Koene
// License TBD

/** @file graph2dil.hpp
 * This header file is used for declarations specific to the graph2dil tool.
 * 
 * Functions and classes available here are typically useful to create the data structures
 * needed when producing DIL Hierarchy compatible output from Graph data.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __GRAPH2DIL_HPP.
 */

#ifndef __GRAPH2DIL_HPP
#include "version.hpp"
#define __GRAPH2DIL_HPP (__VERSION_HPP)

// std
#include <memory>
//#include <map>
#include <vector>

// core
#include "Graphtypes.hpp"

using namespace fz;

//typedef std::map<Topic*, std::unique_ptr<Node_Index>> Node_Index_by_Topic; //*** by Index-ID is faster
typedef std::vector<std::unique_ptr<Node_Index>> Node_Index_by_Topic;

#endif // __GRAPH2DIL_HPP
