// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares standard Log access structures and functionss that should be
 * used when a standardized Formalizer C++ program needs Log access.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __LOGACCESS_HPP.
 */

#ifndef __LOGACCESS_HPP
#include "coreversion.hpp"
#define __LOGACCESS_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
#include <memory>

// core

namespace fz {

// Forward definitions
class Node;
class Graph;
class Log;
class Log_chunk;
class Log_entry;

/**
 * A useful data structure for keeping related data together.
 * See, for example, how this is used in fzlog and fzloghtml.
 */
struct entry_data {
    std::string specific_node_id; ///< if empty then same as Log chunk
    Node * node_ptr = nullptr;
    Graph * graph_ptr = nullptr;
    std::unique_ptr<Log> log_ptr; // *** Do make_unique<Log>() after main().
    Log_chunk * c_newest = nullptr;
    time_t newest_chunk_t = -1;
    bool is_open = false;
    Log_entry * e_newest = nullptr;
    uint8_t newest_minor_id = 0;
    std::string utf8_text;
};

/**
 * Query the Log to find specific entry point.
 * 
 * The results are returned in an `entry_data` structure, which includes
 * information about the corresponding Log chunk and about the specified
 * Log entry. It also makes the in-memory Graph available via pointer.
 * 
 * This function sets the following `entry_data` structure variables:
 * 
 *   log_ptr           Unique_ptr receives and owns a Log object.
 *   c_newest          Receives pointer to corresponding Log_chunk object.
 *   c_newest_chunk_t  Set to open time (==chunk ID) of corresponding Log
 *                     chunk.
 *   is_open           True if the corresponding Log chunk is open, false
 *                     otherwise.
 *   e_newest          Receives pointer to the specified Log_entry
 *                     object in the Log chunk.
 *   newest_minor_id   Set to the minor ID of the entry at e_newest.
 * 
 * Note that the behavior of the call to `Log::get_newest_Entry()` that
 * sets e_newest is different here than when called on other Log
 * intervals. Here, only the last Log chunk is loaded from the database,
 * so that there are no entries if that chunk has none. Under other
 * circumstances it would be possible that an empty most recent Log
 * chunk would return as newest Log entry an object for the last entry
 * in the preceding Log chunk.
 * 
 * The `edata.newest_minor_id` must be set to the enumerator of the
 * Log entry within the Log chunk specified by `chunk_id_str`.
 * 
 * @param[in] pa A valid initialization object for database access.
 * @param[in] chunk_id_str The ID string of a Log chunk.
 * @param[out] edata A convenient structure for the related data.
 */
void get_Log_data(Postgres_access & pa, std::string chunk_id_str, entry_data & edata);

/**
 * Query the Log to find most recent entry points.
 * 
 * The results are returned in an `entry_data` structure, which includes
 * information about the most recent Log chunk and about the most recent
 * Log entry. It also makes the in-memory Graph available via pointer.
 * 
 * This function sets the following `entry_data` structure variables:
 * 
 *   log_ptr           Unique_ptr receives and owns a Log object.
 *   c_newest          Receives pointer to most recent Log_chunk object.
 *                     It the Log is completely empty (a new install)
 *                     then this is nullptr.
 *   c_newest_chunk_t  Set to open time (==chunk ID) of most recent Log
 *                     chunk, or `RTt_unspecified` if there are none.
 *   is_open           True if the most recent Log chunk is open, false
 *                     otherwise (including when the Log is empty).
 *   e_newest          Receives pointer to the most recent Log_entry
 *                     object in the Log chunk, nullptr if there is none,
 *                     which is true both if the Log chunk is empty and
 *                     if the Log is empty.
 *   newest_minor_id   Set to the minor ID of the entry at e_newest, or
 *                     0 if there is none.
 * 
 * Note that the behavior of the call to `Log::get_newest_Entry()` that
 * sets e_newest is different here than when called on other Log
 * intervals. Here, only the last Log chunk is loaded from the database,
 * so that there are no entries if that chunk has none. Under other
 * circumstances it would be possible that an empty most recent Log
 * chunk would return as newest Log entry an object for the last entry
 * in the preceding Log chunk.
 * 
 * @param[in] pa A valid initialization object for database access.
 * @param[out] edata A convenient structure for the related data.
 */
void get_newest_Log_data(Postgres_access & pa, entry_data & edata);

}

#endif // __LOGACCESS_HPP
