// Copyright 2024 Randal A. Koene
// License TBD

/** @file Loginfo.hpp
 * This header file declares basic information gathering functions for use with
 * Log data structures.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __LOGINFO_HPP.
 */

#ifndef __LOGINFO_HPP
#include "coreversion.hpp"
#define __LOGINFO_HPP (__COREVERSION_HPP)

// std

// core
#include "Logtypes.hpp"

namespace fz {

/**
 * Use this class to inspect and evaluate the Log to discover probable issues.
 * 
 * Examples:
 * - Gaps between Log chunks.
 * - Log chunk open and close times that overlap.
 * - Incorrect temporal order of Log chunks.
 * - Unclosed Log chunks.
 * - Log chunks with entries that do not enumerate correctly.
 * - Log chunks that are not owned by valid Nodes.
 * - Strange Log chunk times.
 * - Extremely large Log chunks / Log chunks exceeding a day.
 * - Very tiny Log chunks.
 * 
 * Note: Present implementation assumes that the entire Log fits comfortably
 *       in memory.
 */
class LogIssues {
public:
	LogIssues() {}
};

} // namespace fz

#endif // __LOGINFO_HPP
