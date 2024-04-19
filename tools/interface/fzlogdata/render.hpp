// Copyright 2024 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to template rendering part of
 * fzlogdata.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __RENDER_HPP.
 */

#ifndef __RENDER_HPP
#include "version.hpp"
#define __RENDER_HPP (__VERSION_HPP)

// core
#include "Loginfo.hpp"

using namespace fz;

bool render_integrity_issues(LogIssues & logissues);

#endif // __RENDER_HPP
