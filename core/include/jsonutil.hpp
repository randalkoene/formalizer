// Copyright 2025 Randal A. Koene
// License TBD

/**
 * This header file declares utility functions associated with JSON I/O work.
 * 
 * The corresponding source file is at core/lib/jsonutil.cpp.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __JSONUTIL_HPP.
 */

#ifndef __JSONUTIL_HPP
#include "coreversion.hpp"
#define __JSONUTIL_HPP (__COREVERSION_HPP)

// std
#include <string>


namespace fz {

// Prepare a utf-8 string with valid escapes for inclusion in JSON.
std::string escape_for_json(const std::string& s);

} // namespace fz

#endif // __JSONUTIL_HPP
