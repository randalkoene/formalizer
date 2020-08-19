// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the fzserverpq program.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZSERVERPQ_HPP.
 */

#ifndef __FZSERVERPQ_HPP
#include "version.hpp"
#define __FZSERVERPQ_HPP (__VERSION_HPP)

namespace fz {

/// Postgres database name
extern std::string dbname; // provided in fzserver.cpp, initialized to $USER



} // namespace fz

#endif // __FZSERVERPQ_HPP
