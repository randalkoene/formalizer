// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to template rendering part of
 * fzloghtml.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __RENDER_HPP.
 */

#ifndef __RENDER_HPP
#include "version.hpp"
#define __RENDER_HPP (__VERSION_HPP)

#include "fzlogmap.hpp"

using namespace fz;

/*bool render_Log_interval();
bool render_Log_most_recent();*/
bool render_Node_chunk_data();
bool render_Nodes_subset_chunk_data(const std::map<Node_ID_key, Node_Day_Seconds>& node_day_seconds);

#endif // __RENDER_HPP
