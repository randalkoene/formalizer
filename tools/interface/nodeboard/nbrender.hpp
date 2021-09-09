// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the nbrender part of the
 * nodeboard tool.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __NBRENDER_HPP.
 */

#ifndef __NBRENDER_HPP
#include "version.hpp"
#define __NBRENDER_HPP (__VERSION_HPP)

#include "Graphtypes.hpp"

using namespace fz;

bool node_board_render(Graph & graph);

#endif // __NBRENDER_HPP
