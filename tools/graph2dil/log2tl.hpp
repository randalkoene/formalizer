// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the log2tl part of the
 * graph2dil tool.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __LOG2TL_HPP.
 */

#ifndef __LOG2TL_HPP
#include "version.hpp"
#define __LOG2TL_HPP (__VERSION_HPP)

#include <ostream>

#include "Logtypes.hpp"

using namespace fz;

bool interactive_Log2TL_conversion(Graph & graph, Log & log, std::string TLdirectory, std::ostream * o = nullptr);

#endif // __LOG2TL_HPP
