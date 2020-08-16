// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares the Formalizer Core C++ source code version and functions to inspect it.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __VERSION_HPP.
 */

#ifndef __COREVERSION_HPP
#define __COREVERSION_HPP "2.0.0-0.1"

#include <string>

namespace fz {

inline std::string coreversion() {
    return __COREVERSION_HPP;
}

} // namespace fz

#endif // __COREVERSION_HPP
