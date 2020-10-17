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
 * Query the Log to find most recent entry points.
 * 
 * The results are returned in an `entry_data` structure, which includes
 * information about the most recent Log chunk and about the most recent
 * Log entry. It also makes the in-memory Graph available via pointer.
 * 
 * @param[out] edata A convenient structure for the related data.
 */
void get_newest_Log_data(Postgres_access & pa, entry_data & edata);

}

#endif // __LOGACCESS_HPP
