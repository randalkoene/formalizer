// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the logtest part of the
 * dil2graph tool.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __LOGTEST_HPP.
 */

#ifndef __LOGTEST_HPP
#include "version.hpp"
#define __LOGTEST_HPP (__VERSION_HPP)

#include "Logtypes.hpp"

using namespace fz;

extern std::string testfilepath;

bool test_Log_data(Log & log);

#endif // __LOGTEST_HPP
