// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Functions to help parse tokens that identify Graph components in configuration, on
 * command line and elsewhere.
 * 
 * See for example how this is used in `fzgraph` and in `fzedit`.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __GRAPHTOKEN_HPP.
 */

#ifndef __GRAPHTOKEN_HPP
#include "coreversion.hpp"
#define __GRAPHTOKEN_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
#include <vector>

// core
//#include "error.hpp"
#include "Graphbase.hpp"


namespace fz {

time_t interpret_config_targetdate(const std::string & parvalue);

std::vector<std::string> parse_config_topics(const std::string & parvalue);

td_property interpret_config_tdproperty(const std::string & parvalue);

td_pattern interpret_config_tdpattern(const std::string & parvalue);

} // namespace fz

#endif // __GRAPHTOKEN_HPP