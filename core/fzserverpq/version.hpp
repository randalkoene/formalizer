// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares the C++ source code version and functions to inspect it.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __VERSION_HPP.
 */

#ifndef __VERSION_HPP
#define __VERSION_HPP "0.1.0-0.1"

#include <string>

inline std::string version() {
    return __VERSION_HPP;
}

#endif // __VERSION_HPP
