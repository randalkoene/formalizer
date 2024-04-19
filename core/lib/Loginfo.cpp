// Copyright 2024 Randal A. Koene
// License TBD

// std

// core
//#include "error.hpp"
//#include "standard.hpp"
#include "Loginfo.hpp"

namespace fz {

size_t LogIssues::total_chars_in_entries() {
	total_chars_in_entries_content = 0;
	for (const auto & [ entrykey, entryptr ] : log.get_Entries()) {
		total_chars_in_entries_content += entryptr->entrytext_size();
	}
	return total_chars_in_entries_content;
}

/**
 * Collect all unclosed Log chunks other than the newest.
 * 
 * @return The number of unclosed Log chunks other than the newest.
 */
size_t LogIssues::find_unclosed_chunks() {
	time_t t_newest = log.newest_chunk_t();
	for (const auto & [ logkey, logptr ] : log.get_Chunks()) {
		if ((logptr->is_open()) && (logptr->get_open_time() != t_newest)) {
			unclosed_chunks.emplace_back(logptr->get_open_time());
		}
	}
	return unclosed_chunks.size();
}

/**
 * Collect all chunk close times that leave gaps to the next chunk,
 * and all chunk close times that overlap with subsequent open times.
 * 
 * Note: Does not include cases were chunks are not closed.
 * 
 * @return The number of gaps and overlaps found.
 */
size_t LogIssues::find_gaps_and_overlaps() {
	//time_t t_newest = log.newest_chunk_t();
	Log_chunks_Map & chunks_map = log.get_Chunks();
	for (auto it = chunks_map.begin(); it != chunks_map.end(); it++) {
		auto next_it = std::next(it);
		if (next_it != chunks_map.end()) {
			auto logptr = it->second.get();
			if (!logptr->is_open()) {
				if (logptr->get_close_time() < next_it->second->get_open_time()) {
					gaps.emplace_back(logptr->get_open_time());
				} else if (logptr->get_close_time() > next_it->second->get_open_time()) {
					overlaps.emplace_back(logptr->get_open_time());
				}
			}
		}
	}
	return gaps.size()+overlaps.size();
}

/**
 * Collect all chunks where the open-time is greater than the open-time
 * of the next chunks, i.e. chunks that were placed in the map in the
 * wrong order.
 * 
 * @return The number of order errors in the map.
 */
size_t LogIssues::find_temporal_order_errors() {
	Log_chunks_Map & chunks_map = log.get_Chunks();
	for (auto it = chunks_map.begin(); it != chunks_map.end(); it++) {
		auto next_it = std::next(it);
		if (next_it != chunks_map.end()) {
			auto logptr = it->second.get();
			auto nextptr = next_it->second.get();
			if (logptr->get_open_time() > nextptr->get_open_time()) {
				order_errors.emplace_back(logptr->get_open_time());
			}
		}
	}
	return order_errors.size();
}

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
size_t LogIssues::find_entry_enumeration_errors() {
	entries_map_size = log.num_Entries();
	entries_allocated_to_chunks = 0;
	for (const auto & [ logkey, logptr ] : log.get_Chunks()) {
		std::vector<Log_entry *> & entries_vec = logptr->get_entries();
		uint8_t num_entries = entries_vec.size();
		entries_allocated_to_chunks += num_entries;
		for (uint8_t idx = 0; idx < num_entries; idx++) {
			if (entries_vec.at(idx)->get_minor_id() != (idx+1)) {
				entry_enumeration_errors.emplace_back(logptr->get_open_time());
				break;
			}
		}
	}
	return entry_enumeration_errors.size();
}

/**
 * Collect all chunks with Node IDs that do not indicate valid
 * existing Nodes.
 * 
 * @param graph A valid reference to a Graph object.
 * @return The number of chunks with invalid nodes.
 */
size_t LogIssues::find_invalid_nodes(Graph & graph) {
	for (const auto & [ logkey, logptr ] : log.get_Chunks()) {
		if (graph.Node_by_id(logptr->get_NodeID().key())==nullptr) {
			invalid_nodes.emplace_back(logptr->get_open_time());
		}
	}
	return invalid_nodes.size();
}

/**
 * Collect all chunks of very long duration, as defined by a
 * specified threshold.
 * 
 * @param seconds_threshold A threshold expressed in seconds.
 * @return The number of chunks with a duration exceeding the threshold.
 */
size_t LogIssues::find_very_long_chunks(time_t seconds_threshold) {
	for (const auto & [ logkey, logptr ] : log.get_Chunks()) {
		if (!logptr->is_open()) {
			if (logptr->duration_seconds() > seconds_threshold) {
				very_long_chunks.emplace_back(logptr->get_open_time());
			}
		}
	}
	return very_long_chunks.size();
}

/**
 * Collect all chunks of very small duration, less than a minute.
 * 
 * @return The number of chunks with very small duration.
 */
size_t LogIssues::find_very_tiny_chunks() {
	for (const auto & [ logkey, logptr ] : log.get_Chunks()) {
		if (!logptr->is_open()) {
			if (logptr->duration_seconds() < 60) {
				very_tiny_chunks.emplace_back(logptr->get_open_time());
			}
		}
	}
	return very_tiny_chunks.size();
}

/**
 * Collect all chunks with long entries, more than 5000 characters.
 * 
 * @return The number of chunks with long entries.
 */
size_t LogIssues::find_chunks_with_long_entries() {
	for (const auto & [ logkey, logptr ] : log.get_Chunks()) {
		std::vector<Log_entry *> & entries_vec = logptr->get_entries();
		for (auto & entryptr : entries_vec) {
			if (entryptr->entrytext_size() > 5000) {
				chunks_with_long_entries.emplace_back(logptr->get_open_time());
				break;
			}
		}
	}
	return chunks_with_long_entries.size();
}

void LogIssues::collect_all_issues(Graph & graph, time_t seconds_threshold) {
	total_chars_in_entries();
	find_unclosed_chunks();
	find_gaps_and_overlaps();
	find_temporal_order_errors();
	find_entry_enumeration_errors();
	find_invalid_nodes(graph);
	find_very_long_chunks(seconds_threshold);
	find_very_tiny_chunks();
	find_chunks_with_long_entries();
}

} // namespace fz
