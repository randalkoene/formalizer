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
#include <vector>

// core
#include "Graphtypes.hpp"
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
 * - Extremely large Log chunks / Log chunks exceeding a day.
 * - Very tiny Log chunks.
 * 
 * Note: Present implementation assumes that the entire Log fits comfortably
 *       in memory.
 */
class LogIssues {
protected:
	Log & log;

public:
	std::vector<time_t> unclosed_chunks; // Other than the newest chunk.
	std::vector<time_t> gaps;
	std::vector<time_t> overlaps;
	std::vector<time_t> order_errors;
	std::vector<time_t> entry_enumeration_errors;
	std::vector<time_t> invalid_nodes;
	std::vector<time_t> very_long_chunks;
	std::vector<time_t> very_tiny_chunks;
	std::vector<time_t> chunks_with_long_entries;
	size_t total_chars_in_entries_content = 0;
	size_t entries_map_size = 0;
	size_t entries_allocated_to_chunks = 0;

public:
	LogIssues(Log & _log): log(_log) {}

	size_t total_chars_in_entries();

	/**
	 * Collect all unclosed Log chunks other than the newest.
	 * 
	 * @return The number of unclosed Log chunks other than the newest.
	 */
	size_t find_unclosed_chunks();

	/**
	 * Collect all chunk close times that leave gaps to the next chunk,
	 * and all chunk close times that overlap with subsequent open times.
	 * 
	 * Note: Does not include cases were chunks are not closed.
	 * 
	 * @return The number of gaps and overlaps found.
	 */
	size_t find_gaps_and_overlaps();

	/**
	 * Collect all chunks where the open-time is greater than the open-time
	 * of the next chunks, i.e. chunks that were placed in the map in the
	 * wrong order.
	 * 
	 * @return The number of order errors in the map.
	 */
	size_t find_temporal_order_errors();

	/**
	 * Check that all entries are allocated to existing chunks and
	 * find any entry minor ids that are not in a consecutive enumeration
	 * series.
	 * 
	 * Note that the difference between 'entries_map_size' and
	 * 'entries_allocated_to_chunks' will indicate if any entries were
	 * not properly allocated to chunks.
	 * 
	 * @return The number of chunks with entry enumeration errors.
	 */
	size_t find_entry_enumeration_errors();

	/**
	 * Collect all chunks with Node IDs that do not indicate valid
	 * existing Nodes.
	 * 
	 * @return The number of chunks with invalid nodes.
	 */
	size_t find_invalid_nodes(Graph & graph);

	/**
	 * Collect all chunks of very long duration, as defined by a
	 * specified threshold.
	 * 
	 * @param seconds_threshold A threshold expressed in seconds.
	 * @return The number of chunks with a duration exceeding the threshold.
	 */
	size_t find_very_long_chunks(time_t seconds_threshold);

	/**
	 * Collect all chunks of very small duration, less than a minute.
	 * 
	 * @return The number of chunks with very small duration.
	 */
	size_t find_very_tiny_chunks();

	/**
	 * Collect all chunks with long entries, more than 5000 characters.
	 * 
	 * @return The number of chunks with long entries.
	 */
	size_t find_chunks_with_long_entries();

	void collect_all_issues(Graph & graph, time_t seconds_threshold);
};

} // namespace fz

#endif // __LOGINFO_HPP
